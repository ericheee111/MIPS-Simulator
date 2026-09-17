#include "mips/machine.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    if (size < 24) return 0;
    mips::Program program;
    program.initialMemory.assign(64,0); program.hasEntry = true;
    auto word = [data](std::size_t offset) {
        uint32_t value = 0;
        for (unsigned i=0;i<4;++i) value |= uint32_t(data[offset+i]) << (8*i);
        return value;
    };
    for (unsigned r=1;r<3;++r) {
        mips::Instruction li; li.opcode = mips::Opcode::Li; li.rd = r;
        li.source = mips::Source::immediate(word(4*(r-1))); program.instructions.push_back(li);
    }
    mips::Instruction raw;
    raw.opcode = static_cast<mips::Opcode>(data[8]); raw.rd = data[9]; raw.rs = data[10];
    raw.source.kind = static_cast<mips::Source::Kind>(data[11]); raw.source.value = word(12);
    raw.address.base = raw.source; raw.address.offset = mips::signedValue(word(16));
    raw.target = word(20); raw.threeOperand = (data[8] & 128) != 0;
    program.instructions.push_back(raw);
    mips::Machine machine(program); machine.step(); machine.step(); const auto before = machine;
    if (!machine.step()) {
        if (machine.readPC() != before.readPC() || machine.memoryBytes() != before.memoryBytes() ||
            machine.readHI() != before.readHI() || machine.readLO() != before.readLO()) std::abort();
        for (unsigned i=0;i<32;++i) if (machine.readReg(i) != before.readReg(i)) std::abort();
    }
    if (machine.readReg(0) != 0) std::abort();
    return 0;
}
