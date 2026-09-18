#include "catch.hpp"
#include "benchmarks/observer_group.hpp"
#include "mips/controller.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>

using mips_benchmark::ObserverGroup;

TEST_CASE("Benchmark joins waiting observers during startup unwinding", "[benchmark][concurrency]") {
    std::atomic<unsigned> calls{0};
    auto failedStartup = [&] {
        ObserverGroup clients(2);
        clients.launch([&] { ++calls; });
        clients.launch([&] { ++calls; });
        throw std::runtime_error("run or thread startup failed");
    };
    REQUIRE_THROWS_WITH(failedStartup(), "run or thread startup failed");
    REQUIRE(calls.load() == 0);
}

TEST_CASE("Benchmark partial launch failure preserves thread ownership", "[benchmark][concurrency]") {
    std::atomic<unsigned> calls{0};
    auto failedLaunch = [&] {
        ObserverGroup clients(1);
        clients.launch([&] { ++calls; });
        clients.launch([&] { ++calls; }); // deterministic failure after one launch
    };
    REQUIRE_THROWS_AS(failedLaunch(), std::logic_error);
    REQUIRE(calls.load() == 0);
}

TEST_CASE("Benchmark observation rejection is captured not terminated", "[benchmark][concurrency]") {
    std::promise<void> entered;
    auto started = entered.get_future();
    ObserverGroup clients(1);
    clients.launch([&] {
        entered.set_value();
        throw std::runtime_error("execution command queue is full");
    });
    clients.start();
    REQUIRE(started.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
    clients.stopAndJoin();
    REQUIRE(clients.completed() == 0);
    REQUIRE(clients.failures() == 1);
    REQUIRE_THROWS_WITH(clients.rethrowFirstError(), "execution command queue is full");
}

TEST_CASE("Benchmark captures exceptional controller futures", "[benchmark][concurrency]") {
    mips::ControllerOptions options;
    options.faultHook = [](mips::FaultPoint point) {
        if (point == mips::FaultPoint::BeforeReply) throw std::runtime_error("worker reply failed");
    };
    mips::ExecutionController controller(mips::Machine(), options);
    std::promise<void> entered;
    auto started = entered.get_future();
    ObserverGroup clients(1);
    clients.launch([&] {
        entered.set_value();
        controller.request(mips::CommandKind::Observe).get();
    });
    clients.start();
    REQUIRE(started.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
    clients.stopAndJoin();
    REQUIRE(clients.completed() == 0);
    REQUIRE(clients.failures() == 1);
    REQUIRE_THROWS_WITH(clients.rethrowFirstError(), "worker reply failed");
}

TEST_CASE("Benchmark captures non-standard worker exceptions", "[benchmark][concurrency]") {
    std::promise<void> entered;
    auto started = entered.get_future();
    ObserverGroup clients(1);
    clients.launch([&] { entered.set_value(); throw 42; });
    clients.start();
    REQUIRE(started.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
    clients.stopAndJoin();
    REQUIRE(clients.failures() == 1);
    REQUIRE_THROWS_AS(clients.rethrowFirstError(), int);
}

TEST_CASE("Benchmark counts completed observations and joins idempotently", "[benchmark][concurrency]") {
    std::promise<void> reached;
    auto enough = reached.get_future();
    std::atomic<unsigned> count{0};
    ObserverGroup clients(2);
    for (unsigned i=0; i<2; ++i) clients.launch([&] {
        if (++count == 16) reached.set_value();
    });
    clients.start();
    REQUIRE(enough.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
    clients.stopAndJoin();
    clients.stopAndJoin();
    REQUIRE(clients.completed() >= 16);
    REQUIRE(clients.completed() == count.load());
    REQUIRE(clients.failures() == 0);
    REQUIRE_NOTHROW(clients.rethrowFirstError());
}
