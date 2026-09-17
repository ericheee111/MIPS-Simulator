#include "mips/controller.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
namespace {
mips::Machine loop(std::size_t bytes) {
    mips::Program program; program.initialMemory.assign(bytes,0); program.hasEntry = true;
    mips::Instruction instruction; instruction.opcode = mips::Opcode::J;
    program.instructions.push_back(instruction); return mips::Machine(program);
}
}
int main() {
    std::cout << "bytes,pollers,trial,milliseconds,steps,observations\n";
    for (std::size_t bytes : {std::size_t(1024),std::size_t(1048576),std::size_t(16777216)}) {
        for (unsigned readers : {0U,4U}) for (unsigned trial=0;trial<3;++trial) {
            mips::ExecutionController controller(loop(bytes));
            std::atomic<bool> stop{false}, ready{false}; std::atomic<unsigned> count{0};
            std::vector<std::thread> clients;
            for (unsigned i=0;i<readers;++i) clients.emplace_back([&] {
                while (!ready) std::this_thread::yield();
                while (!stop) {
#ifdef MIPS_BENCH_LEGACY
                    controller.snapshot().get();
#else
                    controller.request(mips::CommandKind::Observe).get();
#endif
                    ++count;
                }
            });
#ifdef MIPS_BENCH_LEGACY
            controller.run().get();
#else
            controller.request(mips::CommandKind::Run).get();
#endif
            const auto start = std::chrono::steady_clock::now(); ready = true;
            // Timed workload duration, NOT a command-completion synchronization.
            std::this_thread::sleep_for(std::chrono::milliseconds(250)); stop = true;
            for (auto& client : clients) client.join();
#ifdef MIPS_BENCH_LEGACY
            const auto steps = controller.pause().get().machine.executedSteps();
#else
            const auto steps = controller.request(mips::CommandKind::Pause).get().state.executed;
#endif
            const auto milliseconds = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            std::cout << bytes << ',' << readers << ',' << trial << ',' << milliseconds << ',' << steps << ',' << count.load() << '\n';
        }
    }
}
