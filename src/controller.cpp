#include "mips/controller.hpp"
#include <stdexcept>
#include <utility>
namespace mips {
Snapshot::Snapshot(const Machine& state, bool active, uint64_t id, bool ok, std::string text)
    : machine(state), running(active), sequence(id), accepted(ok), message(std::move(text)) {}
ExecutionController::ExecutionController(Machine initial)
    : machine_(std::move(initial)), worker_(&ExecutionController::work, this) {}
ExecutionController::~ExecutionController() { shutdown(); }
std::future<Snapshot> ExecutionController::submit(Kind kind, std::unique_ptr<Machine> machine) {
    std::unique_ptr<Command> command(new Command(kind));
    command->machine = std::move(machine);
    auto future = command->result.get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) throw std::logic_error("execution controller is stopped");
        if (commands_.size() >= 1024) throw std::runtime_error("execution command queue is full");
        command->sequence = ++sequence_;
        commands_.push_back(std::move(command));
    }
    ready_.notify_one();
    return future;
}
std::future<Snapshot> ExecutionController::load(Machine machine) {
    return submit(Kind::Load, std::unique_ptr<Machine>(new Machine(std::move(machine))));
}
std::future<Snapshot> ExecutionController::step() { return submit(Kind::Step); }
std::future<Snapshot> ExecutionController::run() { return submit(Kind::Run); }
std::future<Snapshot> ExecutionController::pause() { return submit(Kind::Pause); }
std::future<Snapshot> ExecutionController::snapshot() { return submit(Kind::Snapshot); }
void ExecutionController::shutdown() {
    std::lock_guard<std::mutex> joining(joinMutex_);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    ready_.notify_one();
    if (worker_.joinable()) worker_.join();
}
void ExecutionController::apply(Command& command) {
    bool accepted = true;
    std::string message;
    switch (command.kind) {
    case Kind::Load:
        running_ = false;
        machine_ = std::move(*command.machine);
        break;
    case Kind::Step:
        if (running_) {
            accepted = false;
            message = "Error: simulation running. Type break to halt.";
        } else if (!machine_.step()) { accepted = false; message = machine_.error(); }
        break;
    case Kind::Run:
        if (machine_.getStatus() == Status::Error) { accepted = false; message = machine_.error(); }
        else running_ = true;
        break;
    case Kind::Pause: running_ = false; break;
    case Kind::Snapshot: break;
    }
    command.result.set_value(Snapshot(machine_, running_, command.sequence, accepted, message));
}
void ExecutionController::work() noexcept {
    std::unique_ptr<Command> current;
    try {
        for (;;) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [this] { return stopping_ || running_ || !commands_.empty(); });
                if (!commands_.empty()) {
                    current = std::move(commands_.front());
                    commands_.pop_front();
                } else if (stopping_) break;
            }
            if (current) { apply(*current); current.reset(); }
            else if (running_ && !machine_.step()) running_ = false;
        }
    } catch (...) {
        // Never leave a caller waiting on an unfulfilled promise after worker failure.
        const auto error = std::current_exception();
        if (current) current->result.set_exception(error);
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        for (auto& command : commands_) command->result.set_exception(error);
        commands_.clear();
    }
}
}
