#include "catch.hpp"
#include "mips/controller.hpp"
#include "mips/cli.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <atomic>
#include <chrono>
#include <sstream>
using namespace mips;
namespace {
Machine build(const std::string& text, std::size_t bytes = DefaultMemoryBytes) {
    std::istringstream input(text); Parse parser(bytes);
    if (!parser.parse(tokenize(input))) throw std::runtime_error(parser.error());
    return parser.getVM();
}
Machine loop() { return build(".text\nmain:\nj main\n"); }
template<class T> T await(std::future<T> future) {
    if (future.wait_for(std::chrono::seconds(3)) != std::future_status::ready)
        throw std::runtime_error("future did not complete");
    return future.get();
}
Reply stopped(ExecutionController& controller) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    Reply reply;
    do {
        reply = await(controller.request(CommandKind::Observe));
        if (!reply.running) return reply;
        std::this_thread::yield();
    } while (std::chrono::steady_clock::now() < deadline);
    throw std::runtime_error("run did not stop");
}
#ifdef MIPS_ENABLE_TEST_HOOKS
struct Gate {
    std::promise<void> entered, release;
    std::shared_future<void> open = release.get_future().share();
    void hold() {
        entered.set_value();
        if (open.wait_for(std::chrono::seconds(3)) != std::future_status::ready)
            throw std::runtime_error("test gate timeout");
    }
    bool arrived() { return entered.get_future().wait_for(std::chrono::seconds(3)) == std::future_status::ready; }
};
#endif
}
TEST_CASE("construction seals mutable aliases and reset shares the sealed code", "[hardening][machine]") {
    auto seed = build(".data\nx: .word 9\n.text\nmain:\nli $t0, 7\nsw $t0, 0\n");
    auto builder = std::make_shared<Program>(*seed.program());
    Machine sealed(builder);
    const auto sealedCode = sealed.program();
    builder->initialMemory[0] = 99; builder->instructions[0].source = Source::immediate(44);
    builder->textLabels.clear(); builder->instructions.clear();
    REQUIRE(sealed.readMEM(0,1) == 9); REQUIRE(sealed.B_labelExist("main"));
    REQUIRE(sealed.step()); REQUIRE(sealed.readReg(8) == 7); REQUIRE(sealed.step());
    REQUIRE(sealed.readMEM(0,1) == 7);
    sealed.reset(); REQUIRE(sealed.program() == sealedCode);
    REQUIRE(sealed.readMEM(0,1) == 9); REQUIRE(sealed.readReg(8) == 0);
    REQUIRE(sealed.executedSteps() == 0); REQUIRE(sealed.readPC() == 0);
    Machine empty(std::shared_ptr<const Program>{}); empty.reset();
    REQUIRE(empty.diagnostic().code == FaultCode::NoProgram);
}
TEST_CASE("machine diagnostics contain code line PC and memory location", "[hardening][machine]") {
    auto memory = build(".text\nmain:\nsw $zero, 1023\n");
    REQUIRE_FALSE(memory.step()); const auto& d = memory.diagnostic();
    REQUIRE(d.code == FaultCode::Memory); REQUIRE(d.line == 3); REQUIRE(d.pc == 0);
    REQUIRE(d.hasAddress); REQUIRE(d.address == 1023); REQUIRE(d.width == 4);
    auto overflow = build(".text\nmain:\nli $t0, 2147483647\nadd $t0, $t0, 1\n");
    REQUIRE(overflow.step()); REQUIRE_FALSE(overflow.step());
    REQUIRE(overflow.diagnostic().code == FaultCode::Overflow);
    overflow.reset(); REQUIRE(overflow.diagnostic().code == FaultCode::None);
    auto end = loop(); Program invalid = *end.program(); invalid.entry = 99;
    REQUIRE(Machine(invalid).diagnostic().code == FaultCode::Entry);
}
TEST_CASE("light replies are bounded coherent observations not full machine copies", "[hardening][controller]") {
    auto machine = build(".data\n.word 16909060\n.text\nmain:\nli $t0, 7\nj main", MaxMemoryBytes);
    ExecutionController controller(machine);
    auto reply = await(controller.request(CommandKind::Step));
    REQUIRE(reply.state.memory.empty()); REQUIRE(reply.state.memorySize == MaxMemoryBytes);
    REQUIRE(reply.state.pc == 1); REQUIRE(reply.state.registers[8] == 7);
    auto window = await(controller.request(CommandKind::Observe, MemoryWindow(1,3)));
    REQUIRE(window.state.memory == std::vector<uint8_t>{3,2,1});
    REQUIRE(window.state.program == reply.state.program);
    auto bad = await(controller.request(CommandKind::Step, MemoryWindow(MaxMemoryBytes,1)));
    REQUIRE_FALSE(bad.accepted); REQUIRE(bad.state.pc == 1); REQUIRE(bad.state.memory.empty());
    REQUIRE_THROWS_AS(controller.request(CommandKind::Observe, MemoryWindow(0,4097)), std::invalid_argument);
    REQUIRE_THROWS_AS(controller.request(CommandKind::Load), std::invalid_argument);
    auto invalidKind = await(controller.request(static_cast<CommandKind>(999)));
    REQUIRE_FALSE(invalidKind.accepted);
    auto replaced = await(controller.replace(loop(), MemoryWindow(1024,1)));
    REQUIRE_FALSE(replaced.accepted); REQUIRE(replaced.state.memorySize == MaxMemoryBytes);
    auto tail = await(controller.request(CommandKind::Observe, MemoryWindow(MaxMemoryBytes-1,1)));
    REQUIRE(tail.state.memory.size() == 1);
}
TEST_CASE("run until stops before target and unreachable targets exhaust a budget", "[hardening][controller]") {
    auto machine = build(".text\nmain:\nli $t0, 7\nend:\nj end\n");
    ExecutionController controller(machine);
    REQUIRE(await(controller.request(CommandKind::RunUntil,MemoryWindow(),1,20)).accepted);
    auto target = stopped(controller);
    REQUIRE(target.reason == StopReason::TargetReached); REQUIRE(target.state.executed == 1);
    REQUIRE(target.state.pc == 1); REQUIRE(target.state.registers[8] == 7);
    REQUIRE_FALSE(await(controller.request(CommandKind::RunUntil,MemoryWindow(),9,1)).accepted);
    REQUIRE_FALSE(await(controller.request(CommandKind::RunUntil,MemoryWindow(),0,0)).accepted);
    REQUIRE_FALSE(await(controller.request(CommandKind::RunUntil,MemoryWindow(),0,100000001)).accepted);
    REQUIRE(await(controller.request(CommandKind::RunUntil,MemoryWindow(),0,31)).accepted);
    auto budget = stopped(controller);
    REQUIRE(budget.reason == StopReason::StepLimit); REQUIRE(budget.state.executed == 32);
    REQUIRE(budget.state.status == Status::Simulating);
    auto reset = await(controller.request(CommandKind::Reset));
    REQUIRE(reset.state.pc == 0); REQUIRE(reset.state.executed == 0); REQUIRE(reset.state.registers[8] == 0);
    auto same = await(controller.request(CommandKind::RunUntil,MemoryWindow(),0,1));
    REQUIRE_FALSE(same.running); REQUIRE(same.reason == StopReason::TargetReached);
    await(controller.request(CommandKind::Run));
    REQUIRE_FALSE(await(controller.request(CommandKind::RunUntil,MemoryWindow(),1,1)).accepted);
    REQUIRE_FALSE(await(controller.request(CommandKind::Reset)).running);
}
TEST_CASE("run until observes faults and reset clears sticky faults", "[hardening][controller]") {
    ExecutionController controller(build(".text\nmain:\nsw $zero, 1024\nnop\n"));
    await(controller.request(CommandKind::RunUntil,MemoryWindow(),1,10));
    auto fault = stopped(controller);
    REQUIRE(fault.reason == StopReason::Fault); REQUIRE(fault.state.diagnostic.code == FaultCode::Memory);
    REQUIRE_FALSE(await(controller.request(CommandKind::RunUntil,MemoryWindow(),1,10)).accepted);
    REQUIRE(await(controller.request(CommandKind::Reset)).state.status == Status::Simulating);
}
TEST_CASE("invalid controller configuration is rejected before starting a worker", "[hardening][controller]") {
    for (unsigned mode = 0; mode < 6; ++mode) {
        ControllerOptions options;
        if (mode == 0) options.queueCapacity = 0;
        if (mode == 1) options.queueCapacity = 1025;
        if (mode == 2) options.observationBurst = 0;
        if (mode == 3) options.observationBurst = 1025;
        if (mode == 4) options.stepBatch = 0;
        if (mode == 5) options.stepBatch = 4097;
        REQUIRE_THROWS_AS(ExecutionController(loop(),options), std::invalid_argument);
    }
}
#ifdef MIPS_ENABLE_TEST_HOOKS
TEST_CASE("queue capacity rejects only excess submissions and preserves accepted futures", "[hardening][controller][fault-injection]") {
    Gate gate; std::atomic<bool> first{true}; ControllerOptions options;
    options.queueCapacity = 2;
    options.faultHook = [&](FaultPoint point) { if (point == FaultPoint::BeforeCommand && first.exchange(false)) gate.hold(); };
    ExecutionController controller(loop(),options);
    auto active = controller.request(CommandKind::Step);
    const bool entered = gate.arrived();
    auto a = controller.request(CommandKind::Step); auto b = controller.snapshot();
    bool rejected = false;
    try { controller.request(CommandKind::Step); } catch (const std::runtime_error&) { rejected = true; }
    gate.release.set_value();
    REQUIRE(entered); REQUIRE(rejected);
    REQUIRE(await(std::move(active)).state.executed == 1);
    REQUIRE(await(std::move(a)).state.executed == 2);
    REQUIRE(await(std::move(b)).machine.executedSteps() == 2);
}
TEST_CASE("worker command and reply faults fulfil current and all pending promises", "[hardening][controller][fault-injection]") {
    for (auto point : {FaultPoint::BeforeCommand, FaultPoint::BeforeReply}) {
        Gate gate; std::atomic<bool> first{true}; ControllerOptions options;
        options.faultHook = [&](FaultPoint here) {
            if (here == point && first.exchange(false)) { gate.hold(); throw std::runtime_error("injected worker failure"); }
        };
        ExecutionController controller(loop(),options);
        auto current = controller.request(CommandKind::Step);
        const bool entered = gate.arrived();
        auto next = controller.request(CommandKind::Observe); auto legacy = controller.snapshot();
        gate.release.set_value(); controller.shutdown();
        REQUIRE(entered);
        REQUIRE_THROWS_WITH(await(std::move(current)), "injected worker failure");
        REQUIRE_THROWS_WITH(await(std::move(next)), "injected worker failure");
        REQUIRE_THROWS_WITH(await(std::move(legacy)), "injected worker failure");
        REQUIRE_THROWS_AS(controller.request(CommandKind::Observe), std::logic_error);
        controller.shutdown();
    }
}
TEST_CASE("automatic-step failure stops submissions and remains joinable", "[hardening][controller][fault-injection]") {
    Gate gate; ControllerOptions options;
    options.faultHook = [&](FaultPoint point) {
        if (point == FaultPoint::BeforeAutomaticStep) { gate.hold(); throw std::runtime_error("injected step failure"); }
    };
    ExecutionController controller(loop(),options);
    await(controller.request(CommandKind::Run));
    const bool entered = gate.arrived();
    auto pending = controller.request(CommandKind::Pause);
    gate.release.set_value(); controller.shutdown();
    REQUIRE(entered); REQUIRE_THROWS_WITH(await(std::move(pending)), "injected step failure");
}
TEST_CASE("an observation backlog cannot starve run and pause still stays FIFO", "[hardening][controller]") {
    Gate gate; std::atomic<bool> first{true}; ControllerOptions options;
    options.observationBurst = 2; options.stepBatch = 1;
    options.faultHook = [&](FaultPoint point) { if (point == FaultPoint::BeforeReply && first.exchange(false)) gate.hold(); };
    ExecutionController controller(loop(),options);
    auto run = controller.request(CommandKind::Run);
    const bool entered = gate.arrived();
    std::vector<std::future<Reply>> queries;
    for (unsigned i = 0; i < 16; ++i) queries.push_back(controller.request(CommandKind::Observe));
    auto pause = controller.request(CommandKind::Pause);
    auto after = controller.request(CommandKind::Observe);
    gate.release.set_value();
    REQUIRE(entered); REQUIRE(await(std::move(run)).running);
    uint64_t sequence = 0, executed = 0;
    for (auto& query : queries) {
        auto reply = await(std::move(query));
        REQUIRE(reply.sequence > sequence); sequence = reply.sequence; executed = reply.state.executed;
    }
    REQUIRE(executed == 7); // one instruction between each pair of observations
    REQUIRE(await(std::move(pause)).state.executed == executed);
    auto final = await(std::move(after));
    REQUIRE_FALSE(final.running); REQUIRE(final.state.executed == executed);
}
#endif
TEST_CASE("CLI reset and bounded run-to provide deterministic demonstration commands", "[hardening][cli]") {
    std::istringstream input("until end\nprint $t0\nreset\nprint $t0\nuntil missing\nuntil 0 0\nquit\n");
    std::ostringstream output, errors;
    auto machine = build(".text\nmain:\nli $t0, 7\nend:\nj end\n");
    REQUIRE(runCli(machine,input,output,errors) == 0);
    REQUIRE(output.str().find("Target reached at 0x00000001") != std::string::npos);
    REQUIRE(output.str().find("0x00000007") != std::string::npos);
    REQUIRE(output.str().find("0x00000000") != std::string::npos);
    REQUIRE(errors.str().find("unknown target") != std::string::npos);
    REQUIRE(parseOptions({"--version"}).version);
    REQUIRE(parseOptions({"--gui","x.asm","--smoke-test"}).smokeTest);
    REQUIRE_THROWS_AS(parseOptions({"x.asm","--smoke-test"}), std::invalid_argument);
}
