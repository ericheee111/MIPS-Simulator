#ifndef MIPS_CONTROLLER_HPP
#define MIPS_CONTROLLER_HPP
#include "machine.hpp"
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>
#ifdef MIPS_ENABLE_TEST_HOOKS
#include <functional>
#endif
namespace mips {
// Retained full-copy API for existing library clients. New frontends use Reply.
struct Snapshot {
    Machine machine;
    bool running;
    uint64_t sequence;
    bool accepted;
    std::string message;
    Snapshot(const Machine& state, bool active, uint64_t id, bool ok, std::string text);
};
struct MemoryWindow {
    uint64_t address;
    std::size_t size;
    explicit MemoryWindow(uint64_t base = 0, std::size_t bytes = 0) : address(base), size(bytes) {}
};
const std::size_t MaxObservationBytes = 4096;
enum class StopReason { Paused, Running, TargetReached, StepLimit, Fault };
// A coherent bounded observation, taken at the acknowledged instruction boundary.
// No mutable Machine or full-memory copy is required for a control acknowledgement.
struct DebugState {
    std::shared_ptr<const Program> program;
    std::array<uint32_t, 32> registers{};
    uint32_t pc = 0, hi = 0, lo = 0;
    uint64_t executed = 0;
    Status status = Status::Error;
    Diagnostic diagnostic;
    std::string error;
    std::size_t memorySize = 0;
    uint64_t memoryBase = 0;
    std::vector<uint8_t> memory;
};
struct Reply {
    DebugState state;
    bool running = false;
    uint64_t sequence = 0;
    bool accepted = true;
    std::string message;
    StopReason reason = StopReason::Paused;
};
enum class CommandKind { Load, Step, Run, Pause, Observe, Reset, RunUntil };
#ifdef MIPS_ENABLE_TEST_HOOKS
// Only present in test builds. Hooks execute on the worker; do not call back into it.
enum class FaultPoint { BeforeCommand, BeforeReply, BeforeAutomaticStep };
#endif
struct ControllerOptions {
    std::size_t queueCapacity = 1024;
    std::size_t observationBurst = 16;
    std::size_t stepBatch = 64;
#ifdef MIPS_ENABLE_TEST_HOOKS
    std::function<void(FaultPoint)> faultHook;
#endif
};
// The worker exclusively owns mutable execution state. Public methods are safe
// concurrently while the object lives; destruction must not race its callers.
class ExecutionController {
public:
    explicit ExecutionController(Machine initial = Machine(), ControllerOptions options = ControllerOptions());
    ~ExecutionController();
    ExecutionController(const ExecutionController&) = delete;
    ExecutionController& operator=(const ExecutionController&) = delete;
    // Commands stay FIFO. Invalid windows/targets are rejected before side effects.
    std::future<Reply> request(CommandKind kind, MemoryWindow window = MemoryWindow(),
                               std::size_t target = 0, uint64_t stepLimit = 1000000);
    std::future<Reply> replace(Machine machine, MemoryWindow window = MemoryWindow());
    // Compatibility functions explicitly request an expensive full snapshot.
    std::future<Snapshot> load(Machine machine);
    std::future<Snapshot> step();
    std::future<Snapshot> run();
    std::future<Snapshot> pause();
    std::future<Snapshot> snapshot();
    // Reject new submissions, drain accepted commands without background steps,
    // then join. Concurrent explicit shutdown calls are serialized and idempotent.
    void shutdown();
private:
    struct Command {
        CommandKind kind;
        uint64_t sequence = 0;
        bool legacy = false;
        MemoryWindow window;
        std::size_t target = 0;
        uint64_t limit = 0;
        std::unique_ptr<Machine> machine;
        std::promise<Snapshot> oldResult;
        std::promise<Reply> result;
        explicit Command(CommandKind operation) : kind(operation) {}
        void fail(std::exception_ptr error) noexcept;
    };
    static ControllerOptions validate(ControllerOptions options);
    void enqueue(std::unique_ptr<Command> command);
    std::future<Snapshot> legacy(CommandKind kind, std::unique_ptr<Machine> machine = nullptr);
    void work() noexcept;
    void apply(Command& command);
    void automaticStep();
    DebugState observe(MemoryWindow window) const;
    Machine machine_;
    ControllerOptions options_;
    bool running_ = false;
    bool targeted_ = false;
    std::size_t target_ = 0;
    uint64_t remaining_ = 0;
    StopReason reason_ = StopReason::Paused;
    std::mutex mutex_, joinMutex_;
    std::condition_variable ready_;
    std::deque<std::unique_ptr<Command>> commands_;
    uint64_t sequence_ = 0;
    bool stopping_ = false;
    std::thread worker_; // last: all state exists before starting the thread
};
}
#endif
