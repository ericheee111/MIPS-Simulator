#ifndef MIPS_PROGRAM_HPP
#define MIPS_PROGRAM_HPP
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
namespace mips {
enum class Opcode { Lw, Lh, Lb, La, Sw, Sh, Sb, Li, Move, Mfhi, Mflo, Mthi, Mtlo, Add, Addu, Sub, Subu, Mul, Mulo, Mulou, Rem, Remu, And, Nor, Or, Xor, Mult, Multu, Abs, Neg, Negu, Div, Divu, Not, Beq, Bne, Blt, Ble, Bgt, Bge, J, Nop };
enum class Form { None, Memory, Immediate, SingleRegister, RegisterPair, RegRegSource, Divide, Source, Branch, Jump };
struct OpInfo { Opcode opcode; const char* name; Form form; bool executable; };
const std::vector<OpInfo>& instructionSet();
const OpInfo* findOpcode(const std::string& name);
const OpInfo* opcodeInfo(Opcode opcode);
struct Source {
    enum class Kind { Immediate, Register };
    Kind kind = Kind::Immediate;
    uint32_t value = 0;
    static Source immediate(uint32_t value);
    static Source reg(unsigned index);
};
struct MemoryRef { Source base; int64_t offset = 0; };
struct Instruction {
    Opcode opcode = Opcode::Nop;
    unsigned rd = 0;
    unsigned rs = 0;
    Source source;
    MemoryRef address;
    std::size_t target = 0;
    std::size_t line = 1;
    bool threeOperand = false;
    std::size_t readLineNum() const { return line; }
};
// A builder value. Parsers publish a shared_ptr<const Program> only after success.
struct Program {
    std::vector<Instruction> instructions;
    std::vector<uint8_t> initialMemory;
    std::unordered_map<std::string, std::size_t> textLabels;
    std::unordered_map<std::string, std::size_t> dataLabels;
    std::size_t entry = 0;
    bool hasEntry = false;
};
}
#endif
