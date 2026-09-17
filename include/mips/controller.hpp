#ifndef MIPS_CONTROLLER_HPP
#define MIPS_CONTROLLER_HPP
#include "machine.hpp"
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>
namespace mips {
struct Snapshot {
    Machine machine;
    bool running;
    uint64_t sequence;
    bool accepted;
    std::string message;
    Snapshot(const Machine& state, bool active, uint64_t id, bool ok, std::string text);
};
// FIFO commands complete AFTER their effects, with an immutable-by-copy snapshot.
// The worker exclusively owns machine_ and running_. All public requests may be
// called concurrently while the controller lives. Destruction must not race callers.
class ExecutionController {
public:
    explicit ExecutionController(Machine initial = Machine());
    ~ExecutionController();
    ExecutionController(const ExecutionController&) = delete;
    ExecutionController& operator=(const ExecutionController&) = delete;
    std::future<Snapshot> load(Machine machine);
    std::future<Snapshot> step();
    std::future<Snapshot> run();
    std::future<Snapshot> pause();
    std::future<Snapshot> snapshot();
    // Reject new requests, drain accepted requests, then stop and join. Idempotent.
    void shutdown();
private:
    enum class Kind { Load, Step, Run, Pause, Snapshot };
    struct Command {
        Kind kind;
        uint64_t sequence = 0;
        std::unique_ptr<Machine> machine;
        std::promise<Snapshot> result;
        explicit Command(Kind operation) : kind(operation) {}
    };
    std::future<Snapshot> submit(Kind kind, std::unique_ptr<Machine> machine = nullptr);
    void work() noexcept;
    void apply(Command& command);
    Machine machine_;
    bool running_ = false; // worker only
    std::mutex mutex_, joinMutex_;
    std::condition_variable ready_;
    std::deque<std::unique_ptr<Command>> commands_;
    uint64_t sequence_ = 0; // mutex_
    bool stopping_ = false; // mutex_
    std::thread worker_; // declared last: all state exists before starting it
};
}
#endif
