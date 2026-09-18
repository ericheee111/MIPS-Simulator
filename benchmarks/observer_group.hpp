#ifndef MIPS_BENCHMARK_OBSERVER_GROUP_HPP
#define MIPS_BENCHMARK_OBSERVER_GROUP_HPP
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>

namespace mips_benchmark {
// The owner calls launch/start/stopAndJoin. Observers only call their callback.
// Keep the controller and callback captures alive until this group has joined.
class ObserverGroup {
public:
    explicit ObserverGroup(std::size_t capacity) : errors_(capacity) {
        threads_.reserve(capacity);
    }
    ~ObserverGroup() { stopAndJoin(); }
    ObserverGroup(const ObserverGroup&) = delete;
    ObserverGroup& operator=(const ObserverGroup&) = delete;

    template<class Observe> void launch(Observe observe) {
        if (ready_.load() || stop_.load() || threads_.size() == errors_.size())
            throw std::logic_error("observer launch outside the startup phase");
        const std::size_t index = threads_.size();
        // If construction fails, already-created threads remain owned by this
        // object and are joined during stack unwinding, even before start().
        threads_.emplace_back([this, observe, index] {
            try {
                while (!ready_.load() && !stop_.load()) std::this_thread::yield();
                while (!stop_.load()) {
                    observe();
                    ++completed_;
                }
            } catch (...) {
                // Each worker has its own slot; the owner reads only after join.
                errors_[index] = std::current_exception();
                stop_.store(true);
            }
        });
    }
    void start() noexcept { ready_.store(true); }
    void stopAndJoin() {
        stop_.store(true);
        for (auto& thread : threads_) if (thread.joinable()) thread.join();
    }
    std::uint64_t completed() const noexcept { return completed_.load(); }
    // These two methods require stopAndJoin() first.
    std::size_t failures() const noexcept {
        std::size_t count = 0;
        for (const auto& error : errors_) if (error) ++count;
        return count;
    }
    void rethrowFirstError() const {
        for (const auto& error : errors_) if (error) std::rethrow_exception(error);
    }
private:
    std::atomic<bool> ready_{false}, stop_{false};
    std::atomic<std::uint64_t> completed_{0};
    std::vector<std::exception_ptr> errors_;
    std::vector<std::thread> threads_;
};
}
#endif
