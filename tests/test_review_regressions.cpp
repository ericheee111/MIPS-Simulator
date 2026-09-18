#include "catch.hpp"
#include "mips/controller.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
mips::Program program(mips::Opcode opcode) {
    mips::Program result;
    result.hasEntry = true;
    result.initialMemory.assign(16, 0x5a);
    mips::Instruction instruction;
    instruction.opcode = opcode; instruction.rd = 8; instruction.line = 41;
    result.instructions.push_back(instruction);
    return result;
}
template<class T> T await(std::future<T> future) {
    if (future.wait_for(std::chrono::seconds(3)) != std::future_status::ready)
        throw std::runtime_error("review regression future timed out");
    return future.get();
}
}

TEST_CASE("Invalid address arithmetic reports Address and is failure atomic", "[review][machine]") {
    for (auto opcode : {mips::Opcode::Lw, mips::Opcode::Sw, mips::Opcode::La}) {
        for (unsigned mode = 0; mode < 3; ++mode) {
            auto input = program(opcode);
            auto& address = input.instructions[0].address;
            address.base = mips::Source::immediate(mode == 1 ? UINT32_MAX : 0);
            address.offset = mode == 0 ? -1 : mode == 1 ? 1 : INT64_MAX;
            mips::Machine machine(input);
            REQUIRE_FALSE(machine.step());
            REQUIRE(machine.diagnostic().code == mips::FaultCode::Address);
            REQUIRE(machine.diagnostic().line == 41);
            REQUIRE(machine.diagnostic().pc == 0);
            REQUIRE(machine.readPC() == 0);
            REQUIRE(machine.executedSteps() == 0);
            REQUIRE(machine.readReg(8) == 0);
            REQUIRE(machine.readMEM(0, 4) == 0x5a5a5a5aU);
        }
    }
}

TEST_CASE("Address validation does not swallow Operand or Memory classifications", "[review][machine]") {
    for (auto opcode : {mips::Opcode::Lw, mips::Opcode::Sw, mips::Opcode::La}) {
        auto input = program(opcode);
        input.instructions[0].address.base = mips::Source::immediate(16);
        mips::Machine bounds(input);
        REQUIRE_FALSE(bounds.step());
        REQUIRE(bounds.diagnostic().code == (opcode == mips::Opcode::La
            ? mips::FaultCode::Address : mips::FaultCode::Memory));
        REQUIRE(bounds.executedSteps() == 0);
        input.instructions[0].address.base = mips::Source::reg(99);
        mips::Machine operand(input);
        REQUIRE_FALSE(operand.step());
        REQUIRE(operand.diagnostic().code == mips::FaultCode::Operand);
    }
}

TEST_CASE("Only taken invalid control-flow targets report ProgramCounter", "[review][machine]") {
    for (auto opcode : {mips::Opcode::J, mips::Opcode::Beq, mips::Opcode::Bne,
                        mips::Opcode::Blt, mips::Opcode::Ble, mips::Opcode::Bgt, mips::Opcode::Bge}) {
        for (bool take : {false, true}) {
            if (opcode == mips::Opcode::J && !take) continue;
            auto input = program(opcode);
            input.instructions[0].target = 99;
            uint32_t operand = 0; // rs is register zero
            switch (opcode) {
            case mips::Opcode::Beq: case mips::Opcode::Bge: operand = take ? 0 : 1; break;
            case mips::Opcode::Bne: case mips::Opcode::Blt: operand = take ? 1 : 0; break;
            case mips::Opcode::Ble: operand = take ? 0 : UINT32_MAX; break;
            case mips::Opcode::Bgt: operand = take ? UINT32_MAX : 0; break;
            default: break;
            }
            input.instructions[0].source = mips::Source::immediate(operand);
            mips::Machine machine(input);
            REQUIRE(machine.step() == !take);
            REQUIRE(machine.diagnostic().code == (take ? mips::FaultCode::ProgramCounter : mips::FaultCode::None));
            REQUIRE(machine.readPC() == (take ? 0U : 1U));
            REQUIRE(machine.executedSteps() == (take ? 0U : 1U));
            REQUIRE(machine.readReg(8) == 0);
            REQUIRE(machine.readMEM(0,4) == 0x5a5a5a5aU);
        }
    }
}

TEST_CASE("Destroying a consumer future leaves its provider usable", "[review][concurrency]") {
    std::promise<int> provider;
    { auto consumer = provider.get_future(); REQUIRE(consumer.valid()); }
    REQUIRE_NOTHROW(provider.set_value(7));
}

TEST_CASE("Discarded pending replies do not stop the controller or break reload", "[review][controller]") {
    std::promise<void> entered, release;
    auto open = release.get_future().share();
    std::atomic<bool> first{true};
    mips::ControllerOptions options;
    options.faultHook = [&](mips::FaultPoint point) {
        if (point == mips::FaultPoint::BeforeCommand && first.exchange(false)) {
            entered.set_value();
            if (open.wait_for(std::chrono::seconds(3)) != std::future_status::ready)
                throw std::runtime_error("review regression gate timed out");
        }
    };
    mips::ExecutionController controller(mips::Machine(program(mips::Opcode::J)), options);
    auto active = controller.request(mips::CommandKind::Observe);
    const bool blocked = entered.get_future().wait_for(std::chrono::seconds(3)) == std::future_status::ready;
    std::vector<std::future<mips::Reply>> replies;
    std::vector<std::future<mips::Snapshot>> snapshots;
    for (unsigned i = 0; i < 4; ++i) {
        replies.push_back(controller.request(mips::CommandKind::Observe));
        snapshots.push_back(controller.snapshot());
    }
    replies.clear(); snapshots.clear(); // definitely before their promises are fulfilled
    auto barrier = controller.request(mips::CommandKind::Pause);
    release.set_value();
    REQUIRE(blocked);
    REQUIRE(await(std::move(active)).accepted);
    REQUIRE(await(std::move(barrier)).accepted);
    auto replacement = program(mips::Opcode::Li);
    replacement.instructions[0].source = mips::Source::immediate(9);
    REQUIRE(await(controller.replace(mips::Machine(replacement))).accepted);
    const auto step = await(controller.request(mips::CommandKind::Step));
    REQUIRE(step.accepted);
    REQUIRE(step.state.registers[8] == 9);
    REQUIRE(step.state.executed == 1);
}
