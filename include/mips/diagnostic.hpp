#ifndef MIPS_DIAGNOSTIC_HPP
#define MIPS_DIAGNOSTIC_HPP
#include <cstddef>
#include <cstdint>
#include <string>
namespace mips {
// Structured machine faults; frontends may retain the legacy Error:<line>: text.
enum class FaultCode { None, NoProgram, Entry, ProgramCounter, Unsupported,
                       Operand, Address, Memory, Overflow };
struct Diagnostic {
    FaultCode code = FaultCode::None;
    std::size_t line = 0;
    uint32_t pc = 0;
    bool hasAddress = false;
    uint64_t address = 0;
    unsigned width = 0;
    std::string message;
};
}
#endif
