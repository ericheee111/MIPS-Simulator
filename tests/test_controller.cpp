#include "catch.hpp"
#include "mips/controller.hpp"
#include "mips/cli.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <sstream>
#include <atomic>
#include <chrono>
using namespace mips;
namespace {
Machine machine(const std::string& code) {
    std::istringstream source(".text\nmain:\n" + code); Parse parser;
    if (!parser.parse(tokenize(source))) throw std::runtime_error(parser.error());
    return parser.getVM();
}
Snapshot completed(std::future<Snapshot> result) {
    if (result.wait_for(std::chrono::seconds(3)) != std::future_status::ready)
        throw std::runtime_error("controller acknowledgement timeout");
    return result.get();
}
}
TEST_CASE("load does not execute and FIFO steps acknowledge exact results", "[controller]") {
    ExecutionController controller(machine("addu $t0, $t0, 1\nj main"));
    REQUIRE(completed(controller.snapshot()).machine.readReg(8) == 0);
    auto first = controller.step(); auto second = controller.step(); auto third = controller.step();
    auto a = completed(std::move(first)); auto b = completed(std::move(second)); auto c = completed(std::move(third));
    REQUIRE(a.machine.readPC() == 1); REQUIRE(b.machine.readPC() == 0); REQUIRE(c.machine.readPC() == 1);
    REQUIRE(a.sequence < b.sequence); REQUIRE(b.sequence < c.sequence);
    REQUIRE(a.machine.readReg(8) == 1); REQUIRE(c.machine.readReg(8) == 2);
    auto reloaded = completed(controller.load(machine("li $t0, 77")));
    REQUIRE(reloaded.machine.readPC() == 0); REQUIRE(reloaded.machine.readReg(8) == 0);
}
TEST_CASE("run is idempotent and pause never consumes a subsequent command", "[controller]") {
    ExecutionController controller(machine("j main"));
    for (unsigned i = 0; i < 100; ++i) {
        REQUIRE(completed(controller.run()).running);
        REQUIRE(completed(controller.run()).running);
        auto rejected = completed(controller.step());
        REQUIRE_FALSE(rejected.accepted);
        REQUIRE(rejected.message == "Error: simulation running. Type break to halt.");
        auto paused = controller.pause(); auto after = controller.snapshot();
        REQUIRE_FALSE(completed(std::move(paused)).running);
        REQUIRE_FALSE(completed(std::move(after)).running);
    }
    auto pause = controller.pause(); auto last = controller.snapshot();
    controller.shutdown(); // drains both, does not eat the next message
    REQUIRE_FALSE(completed(std::move(pause)).running);
    REQUIRE_FALSE(completed(std::move(last)).running);
    REQUIRE_THROWS_AS(controller.step(), std::logic_error);
    controller.shutdown();
}
TEST_CASE("runtime faults stop running and remain inspectable", "[controller]") {
    ExecutionController controller(machine("li $t0, 1\nlw $t0, 1024"));
    completed(controller.run());
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    auto state = completed(controller.snapshot());
    while (state.running && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield(); state = completed(controller.snapshot());
    }
    REQUIRE_FALSE(state.running); REQUIRE(state.machine.getStatus() == Status::Error);
    REQUIRE(state.machine.readPC() == 1); REQUIRE(state.machine.readReg(8) == 1);
    REQUIRE_FALSE(completed(controller.run()).accepted);
}
TEST_CASE("concurrent callers observe independent ordered snapshots", "[controller]") {
    ExecutionController controller(machine("addu $t0, $t0, 1\nj main"));
    std::atomic<unsigned> failures{0};
    std::vector<std::thread> callers;
    for (unsigned t = 0; t < 4; ++t) callers.emplace_back([&] {
        try {
            for (unsigned i = 0; i < 100; ++i) {
                auto state = completed(controller.step());
                if (!state.accepted || state.machine.readReg(8) != (state.machine.executedSteps() + 1) / 2) ++failures;
            }
        } catch (...) { ++failures; }
    });
    for (auto& thread : callers) thread.join();
    REQUIRE(failures == 0);
    auto state = completed(controller.snapshot());
    REQUIRE(state.machine.executedSteps() == 400); REQUIRE(state.machine.readReg(8) == 200);
}
TEST_CASE("shutdown drains accepted futures and joins idle or active workers", "[controller]") {
    for (unsigned mode = 0; mode < 2; ++mode) {
        ExecutionController controller(machine("j main"));
        if (mode) completed(controller.run());
        auto pause = controller.pause(); auto snapshot = controller.snapshot();
        std::thread a([&] { controller.shutdown(); });
        std::thread b([&] { controller.shutdown(); });
        a.join(); b.join();
        REQUIRE_FALSE(completed(std::move(pause)).running);
        REQUIRE_FALSE(completed(std::move(snapshot)).running);
    }
    for (unsigned i = 0; i < 25; ++i) { ExecutionController active(machine("j main")); completed(active.run()); }
}
TEST_CASE("CLI options and command errors are bounded and deterministic", "[cli]") {
    REQUIRE(parseOptions({"--gui", "a.asm"}).filename == "a.asm");
    REQUIRE(parseOptions({"a.asm", "--gui"}).gui);
    REQUIRE(parseOptions({"--", "-a.asm"}).filename == "-a.asm");
    REQUIRE(parseOptions({"--help"}).help);
    for (auto args : std::vector<std::vector<std::string>>{{}, {"--gui"}, {"a", "b"}, {"--bad"}, {"a", "--gui", "--gui"}})
        REQUIRE_THROWS_AS(parseOptions(args), std::invalid_argument);
    std::istringstream input("print $pc\nstep\nprint $t0\nprint\nprint $\nprint &0x\nprint &0xffffffff\nquit\n");
    std::ostringstream out, err;
    REQUIRE(runCli(machine("li $t0, 7\nj main"),input,out,err) == 0);
    REQUIRE(out.str().find("0x00000000") != std::string::npos);
    REQUIRE(out.str().find("0x00000007") != std::string::npos);
    REQUIRE(err.str().find("out of bounds") != std::string::npos);
}
TEST_CASE("CLI EOF stops active execution and running-state commands are rejected", "[cli]") {
    std::istringstream input("run\nprint $t0\nstep\nbreak\nrun\n"); // EOF, no quit
    std::ostringstream out, err;
    REQUIRE(runCli(machine("j main"),input,out,err) == 0);
    REQUIRE(err.str() == "Error: simulation running. Type break to halt.\nError: simulation running. Type break to halt.\n");
}
