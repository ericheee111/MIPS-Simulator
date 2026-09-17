#include "mips/controller.hpp"
#include <stdexcept>
#include <utility>
namespace mips {
Snapshot::Snapshot(const Machine& state, bool active, uint64_t id, bool ok, std::string text)
    : machine(state), running(active), sequence(id), accepted(ok), message(std::move(text)) {}
ControllerOptions ExecutionController::validate(ControllerOptions options) {
    if (options.queueCapacity == 0 || options.queueCapacity > 1024 ||
        options.observationBurst == 0 || options.observationBurst > 1024 ||
        options.stepBatch == 0 || options.stepBatch > 4096)
        throw std::invalid_argument("invalid controller queue/burst/batch bound");
    return options;
}
ExecutionController::ExecutionController(Machine initial, ControllerOptions options)
    : machine_(std::move(initial)), options_(validate(std::move(options))),
      worker_(&ExecutionController::work, this) {}
ExecutionController::~ExecutionController() { shutdown(); }
void ExecutionController::Command::fail(std::exception_ptr error) noexcept {
    try {
        if (legacy) oldResult.set_exception(error);
        else result.set_exception(error);
    } catch (...) { /* A destroyed promise still wakes its future as broken_promise. */ }
}
void ExecutionController::enqueue(std::unique_ptr<Command> command) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) throw std::logic_error("execution controller is stopped");
        if (commands_.size() >= options_.queueCapacity) throw std::runtime_error("execution command queue is full");
        command->sequence = ++sequence_;
        commands_.push_back(std::move(command));
    }
    ready_.notify_one();
}
std::future<Reply> ExecutionController::request(CommandKind kind, MemoryWindow window,
                                               std::size_t target, uint64_t stepLimit) {
    if (kind == CommandKind::Load) throw std::invalid_argument("use replace to load a machine");
    if (window.size > MaxObservationBytes) throw std::invalid_argument("observation window exceeds 4096 bytes");
    std::unique_ptr<Command> command(new Command(kind));
    command->window = window; command->target = target; command->limit = stepLimit;
    auto future = command->result.get_future();
    enqueue(std::move(command));
    return future;
}
std::future<Reply> ExecutionController::replace(Machine machine, MemoryWindow window) {
    if (window.size > MaxObservationBytes) throw std::invalid_argument("observation window exceeds 4096 bytes");
    std::unique_ptr<Command> command(new Command(CommandKind::Load));
    command->window = window;
    command->machine.reset(new Machine(std::move(machine)));
    auto future = command->result.get_future();
    enqueue(std::move(command));
    return future;
}
std::future<Snapshot> ExecutionController::legacy(CommandKind kind, std::unique_ptr<Machine> machine) {
    std::unique_ptr<Command> command(new Command(kind));
    command->legacy = true; command->machine = std::move(machine);
    auto future = command->oldResult.get_future();
    enqueue(std::move(command));
    return future;
}
std::future<Snapshot> ExecutionController::load(Machine machine) {
    return legacy(CommandKind::Load, std::unique_ptr<Machine>(new Machine(std::move(machine))));
}
std::future<Snapshot> ExecutionController::step() { return legacy(CommandKind::Step); }
std::future<Snapshot> ExecutionController::run() { return legacy(CommandKind::Run); }
std::future<Snapshot> ExecutionController::pause() { return legacy(CommandKind::Pause); }
std::future<Snapshot> ExecutionController::snapshot() { return legacy(CommandKind::Observe); }
void ExecutionController::shutdown() {
    std::lock_guard<std::mutex> joining(joinMutex_);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    ready_.notify_one();
    if (worker_.joinable()) worker_.join();
}
DebugState ExecutionController::observe(MemoryWindow window) const {
    DebugState state;
    state.program = machine_.program(); state.pc = machine_.readPC();
    state.hi = machine_.readHI(); state.lo = machine_.readLO();
    state.executed = machine_.executedSteps(); state.status = machine_.getStatus();
    state.error = machine_.error(); state.diagnostic = machine_.diagnostic();
    for (unsigned i = 0; i < 32; ++i) state.registers[i] = machine_.readReg(i);
    state.memorySize = machine_.memSize(); state.memoryBase = window.address;
    const auto& bytes = machine_.memoryBytes();
    if (window.size != 0) {
        const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(window.address);
        state.memory.assign(begin, begin + static_cast<std::ptrdiff_t>(window.size));
    }
    return state;
}
void ExecutionController::apply(Command& command) {
#ifdef MIPS_ENABLE_TEST_HOOKS
    if (options_.faultHook) options_.faultHook(FaultPoint::BeforeCommand);
#endif
    bool accepted = true;
    std::string message;
    const auto memorySize = command.kind == CommandKind::Load ? command.machine->memSize() : machine_.memSize();
    if (!command.legacy && (command.window.size > memorySize ||
        command.window.address > memorySize - command.window.size)) {
        accepted = false; message = "Error: observation window out of bounds";
    }
    if (accepted) switch (command.kind) {
    case CommandKind::Load:
        running_ = targeted_ = false; reason_ = StopReason::Paused;
        machine_ = std::move(*command.machine);
        break;
    case CommandKind::Reset:
        machine_.reset(); running_ = targeted_ = false; reason_ = StopReason::Paused;
        break;
    case CommandKind::Step:
        if (running_) { accepted = false; message = "Error: simulation running. Type break to halt."; }
        else if (!machine_.step()) { accepted = false; message = machine_.error(); reason_ = StopReason::Fault; }
        else reason_ = StopReason::Paused;
        break;
    case CommandKind::Run:
        if (machine_.getStatus() == Status::Error) { accepted = false; message = machine_.error(); }
        else if (!running_) { running_ = true; targeted_ = false; reason_ = StopReason::Running; }
        break;
    case CommandKind::RunUntil:
        if (running_) { accepted = false; message = "Error: simulation running. Type break to halt."; }
        else if (machine_.getStatus() == Status::Error) { accepted = false; message = machine_.error(); }
        else if (command.target >= machine_.getInstrVector().size() || command.limit == 0 || command.limit > 100000000) {
            accepted = false; message = "Error: invalid run-until target or instruction budget";
        } else {
            target_ = command.target; remaining_ = command.limit; targeted_ = true;
            running_ = machine_.readPC() != target_;
            reason_ = running_ ? StopReason::Running : StopReason::TargetReached;
        }
        break;
    case CommandKind::Pause:
        running_ = targeted_ = false;
        if (reason_ == StopReason::Running) reason_ = StopReason::Paused;
        break;
    case CommandKind::Observe: break;
    default: accepted = false; message = "Error: invalid execution command"; break;
    }
#ifdef MIPS_ENABLE_TEST_HOOKS
    if (options_.faultHook) options_.faultHook(FaultPoint::BeforeReply);
#endif
    if (command.legacy) command.oldResult.set_value(Snapshot(machine_, running_, command.sequence, accepted, message));
    else {
        Reply reply;
        reply.state = observe(accepted ? command.window : MemoryWindow());
        reply.running = running_; reply.sequence = command.sequence; reply.accepted = accepted;
        reply.message = std::move(message); reply.reason = reason_;
        command.result.set_value(std::move(reply));
    }
}
void ExecutionController::automaticStep() {
#ifdef MIPS_ENABLE_TEST_HOOKS
    if (options_.faultHook) options_.faultHook(FaultPoint::BeforeAutomaticStep);
#endif
    if (!machine_.step()) { running_ = false; reason_ = StopReason::Fault; return; }
    if (targeted_) {
        --remaining_;
        if (machine_.readPC() == target_) { running_ = false; reason_ = StopReason::TargetReached; }
        else if (remaining_ == 0) { running_ = false; reason_ = StopReason::StepLimit; }
    }
}
void ExecutionController::work() noexcept {
    std::unique_ptr<Command> current;
    std::size_t observations = 0;
    try {
        for (;;) {
            bool background = false;
            std::size_t batch = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [this] { return stopping_ || running_ || !commands_.empty(); });
                if (!commands_.empty()) {
                    // Interleave one instruction after a bounded burst of queries.
                    // Never bypass a control command at the front or run during drain.
                    if (!stopping_ && running_ && observations >= options_.observationBurst &&
                        commands_.front()->kind == CommandKind::Observe) {
                        background = true; batch = 1; observations = 0;
                    } else {
                        current = std::move(commands_.front()); commands_.pop_front();
                    }
                } else if (stopping_) break;
                else { background = true; batch = options_.stepBatch; observations = 0; }
            }
            if (current) {
                apply(*current);
                if (current->kind == CommandKind::Observe) ++observations; else observations = 0;
                current.reset();
            } else if (background) {
                for (std::size_t i = 0; i < batch && running_; ++i) automaticStep();
            }
        }
        running_ = false;
    } catch (...) {
        const auto error = std::current_exception();
        if (current) current->fail(error);
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        for (auto& command : commands_) command->fail(error);
        commands_.clear();
    }
}
}
