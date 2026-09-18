#include "mips/program.hpp"
namespace mips {
const std::vector<OpInfo>& instructionSet() {
    static const std::vector<OpInfo> table = {
        {Opcode::Lw, "lw", Form::Memory, true},
        {Opcode::Lh, "lh", Form::Memory, false},
        {Opcode::Lb, "lb", Form::Memory, false},
        {Opcode::La, "la", Form::Memory, true},
        {Opcode::Sw, "sw", Form::Memory, true},
        {Opcode::Sh, "sh", Form::Memory, false},
        {Opcode::Sb, "sb", Form::Memory, false},
        {Opcode::Li, "li", Form::Immediate, true},
        {Opcode::Move, "move", Form::RegisterPair, true},
        {Opcode::Mfhi, "mfhi", Form::SingleRegister, true},
        {Opcode::Mflo, "mflo", Form::SingleRegister, true},
        {Opcode::Mthi, "mthi", Form::SingleRegister, false},
        {Opcode::Mtlo, "mtlo", Form::SingleRegister, false},
        {Opcode::Add, "add", Form::RegRegSource, true},
        {Opcode::Addu, "addu", Form::RegRegSource, true},
        {Opcode::Sub, "sub", Form::RegRegSource, true},
        {Opcode::Subu, "subu", Form::RegRegSource, true},
        {Opcode::Mul, "mul", Form::RegRegSource, false},
        {Opcode::Mulo, "mulo", Form::RegRegSource, false},
        {Opcode::Mulou, "mulou", Form::RegRegSource, false},
        {Opcode::Rem, "rem", Form::RegRegSource, false},
        {Opcode::Remu, "remu", Form::RegRegSource, false},
        {Opcode::And, "and", Form::RegRegSource, true},
        {Opcode::Nor, "nor", Form::RegRegSource, true},
        {Opcode::Or, "or", Form::RegRegSource, true},
        {Opcode::Xor, "xor", Form::RegRegSource, true},
        {Opcode::Mult, "mult", Form::RegisterPair, true},
        {Opcode::Multu, "multu", Form::RegisterPair, true},
        {Opcode::Abs, "abs", Form::RegisterPair, false},
        {Opcode::Neg, "neg", Form::RegisterPair, false},
        {Opcode::Negu, "negu", Form::RegisterPair, false},
        {Opcode::Div, "div", Form::Divide, true},
        {Opcode::Divu, "divu", Form::Divide, true},
        {Opcode::Not, "not", Form::Source, true},
        {Opcode::Beq, "beq", Form::Branch, true},
        {Opcode::Bne, "bne", Form::Branch, true},
        {Opcode::Blt, "blt", Form::Branch, true},
        {Opcode::Ble, "ble", Form::Branch, true},
        {Opcode::Bgt, "bgt", Form::Branch, true},
        {Opcode::Bge, "bge", Form::Branch, true},
        {Opcode::J, "j", Form::Jump, true},
        {Opcode::Nop, "nop", Form::None, true},
    };
    return table;
}
const OpInfo* findOpcode(const std::string& name) {
    for (const auto& info : instructionSet()) if (name == info.name) return &info;
    return nullptr;
}
const OpInfo* opcodeInfo(Opcode opcode) {
    const auto& table = instructionSet();
    const auto index = static_cast<std::size_t>(opcode);
    return index < table.size() && table[index].opcode == opcode ? &table[index] : nullptr;
}
Source Source::immediate(uint32_t value) { Source s; s.value = value; return s; }
Source Source::reg(unsigned index) { Source s; s.kind = Kind::Register; s.value = index; return s; }
}
