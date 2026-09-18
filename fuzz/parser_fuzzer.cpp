#include "lexer.hpp"
#include "parser.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <sstream>
// Build separately with libFuzzer + ASan/UBSan. Never execute unbounded programs.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    if (size > 16384) return 0;
    std::istringstream input(std::string(reinterpret_cast<const char*>(data),size));
    Parse parser(1024);
    if (!parser.parse(tokenize(input))) return 0;
    auto machine = parser.getVM();
    for (unsigned step=0; step<64; ++step) {
        const auto before = machine;
        const bool ok = machine.step();
        if (machine.readReg(0) != 0) std::abort();
        if (!ok) {
            if (machine.readPC() != before.readPC() || machine.memoryBytes() != before.memoryBytes() ||
                machine.readHI() != before.readHI() || machine.readLO() != before.readLO()) std::abort();
            for (unsigned i=0;i<32;++i) if (machine.readReg(i) != before.readReg(i)) std::abort();
            break;
        }
    }
    // Reuse must not retain a successful old program after a failed parse.
    std::istringstream invalid(".text\nmain:\nli $t0, +\n");
    if (parser.parse(tokenize(invalid)) || parser.program()) std::abort();
    return 0;
}
