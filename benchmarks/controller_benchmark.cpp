#include "mips/controller.hpp"
#include "observer_group.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
mips::Machine loop(std::size_t bytes) {
    mips::Program program; program.initialMemory.assign(bytes,0); program.hasEntry = true;
    mips::Instruction instruction; instruction.opcode = mips::Opcode::J;
    program.instructions.push_back(instruction); return mips::Machine(program);
}
void trial(std::size_t bytes, unsigned readers, unsigned number) {
    mips::ExecutionController controller(loop(bytes));
    // Declared after the controller: every exit joins clients before destroying it.
    mips_benchmark::ObserverGroup clients(readers);
    for (unsigned i=0;i<readers;++i) clients.launch([&] {
#ifdef MIPS_BENCH_LEGACY
        const auto reply = controller.snapshot().get();
#else
        const auto reply = controller.request(mips::CommandKind::Observe).get();
#endif
        if (!reply.accepted) throw std::runtime_error("observation rejected: " + reply.message);
    });
#ifdef MIPS_BENCH_LEGACY
    const auto running = controller.run().get();
#else
    const auto running = controller.request(mips::CommandKind::Run).get();
#endif
    if (!running.accepted) throw std::runtime_error("run rejected: " + running.message);
    const auto start = std::chrono::steady_clock::now();
    clients.start();
    // Timed workload duration, NOT a command-completion synchronization.
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    clients.stopAndJoin();
    if (clients.failures()) {
        // Backpressure and future failures invalidate a measurement. Do not hide
        // them with retries or count failed observations as successful throughput.
        std::cerr << "invalid trial: bytes=" << bytes << ", pollers=" << readers
                  << ", trial=" << number << ", completed=" << clients.completed()
                  << ", failed observations=" << clients.failures() << '\n';
        clients.rethrowFirstError();
    }
#ifdef MIPS_BENCH_LEGACY
    const auto paused = controller.pause().get();
    const auto steps = paused.machine.executedSteps();
#else
    const auto paused = controller.request(mips::CommandKind::Pause).get();
    const auto steps = paused.state.executed;
#endif
    if (!paused.accepted) throw std::runtime_error("pause rejected: " + paused.message);
    const auto milliseconds = std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-start).count();
    std::cout << bytes << ',' << readers << ',' << number << ',' << milliseconds
              << ',' << steps << ',' << clients.completed() << '\n';
}
}
int main() {
    try {
        std::cout << "bytes,pollers,trial,milliseconds,steps,observations\n";
        for (std::size_t bytes : {std::size_t(1024),std::size_t(1048576),std::size_t(16777216)})
            for (unsigned readers : {0U,4U})
                for (unsigned number=0;number<3;++number) trial(bytes,readers,number);
    } catch (const std::exception& error) {
        std::cerr << "controller benchmark failed: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "controller benchmark failed: non-standard exception\n";
        return 1;
    }
    return 0;
}
