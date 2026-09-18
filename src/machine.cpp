#include "mips/machine.hpp"
#include <stdexcept>
#include <utility>
namespace mips {
namespace {
// Invalid address arithmetic is distinct from an invalid register/source operand.
class AddressError : public std::out_of_range {
public:
    explicit AddressError(const char* message) : std::out_of_range(message) {}
};
}
Machine::Machine() { initialize(); }
Machine::Machine(const Program& program)
    : program_(std::make_shared<const Program>(program)), memory_(program.initialMemory) {
    initialize();
}
Machine::Machine(std::shared_ptr<const Program> program)
    : program_(program ? std::make_shared<const Program>(*program) : nullptr),
      memory_(program_ ? Memory(program_->initialMemory) : Memory()) {
    initialize();
}
void Machine::initialize() {
    registers_.fill(0); pc_ = program_ ? program_->entry : 0;
    hi_ = lo_ = 0; executed_ = 0; diagnostic_ = Diagnostic();
    if (!program_) { fault(1, "no program loaded", FaultCode::NoProgram); return; }
    if (!program_->hasEntry) { fault(1, "missing main instruction label", FaultCode::Entry); return; }
    if (pc_ >= program_->instructions.size()) { fault(1, "main does not name an instruction", FaultCode::Entry); return; }
    status_ = Status::Simulating;
    error_.clear();
}
void Machine::reset() {
    // Allocate first, so allocation failure leaves the existing machine intact.
    Memory initial = program_ ? Memory(program_->initialMemory) : Memory();
    memory_ = std::move(initial);
    initialize();
}
const std::vector<Instruction>& Machine::getInstrVector() const {
    static const std::vector<Instruction> empty;
    return program_ ? program_->instructions : empty;
}
bool Machine::B_labelExist(const std::string& name) const {
    return program_ && program_->textLabels.count(name) != 0;
}
bool Machine::D_labelExist(const std::string& name) const {
    return program_ && program_->dataLabels.count(name) != 0;
}
void Machine::fault(std::size_t line, const std::string& message, FaultCode code) {
    diagnostic_ = Diagnostic();
    diagnostic_.code = code; diagnostic_.line = line; diagnostic_.pc = readPC();
    diagnostic_.message = message;
    status_ = Status::Error;
    error_ = "Error:" + std::to_string(line) + ": " + message;
}
void Machine::writeRegister(unsigned index, uint32_t value) {
    if (index >= 32) throw std::out_of_range("invalid register index");
    if (index != 0) registers_[index] = value;
}
uint32_t Machine::sourceValue(const Source& source) const {
    if (source.kind == Source::Kind::Register) return readReg(source.value);
    if (source.kind == Source::Kind::Immediate) return source.value;
    throw std::out_of_range("invalid operand kind");
}
uint64_t Machine::addressValue(const MemoryRef& address) const {
    // The parser bounds offsets to int32. Validate public IR too, before signed addition.
    if (address.offset < -2147483648LL || address.offset > 2147483647LL)
        throw AddressError("offset exceeds signed 32-bit range");
    const int64_t effective = static_cast<int64_t>(sourceValue(address.base)) + address.offset;
    if (effective < 0 || effective > 4294967295LL)
        throw AddressError("effective address exceeds 32-bit range");
    return static_cast<uint64_t>(effective);
}
bool Machine::step() {
    if (status_ == Status::Error) return false;
    const auto& code = program_->instructions;
    if (pc_ >= code.size()) { fault(code.empty() ? 1 : code.back().line, "program counter out of bounds", FaultCode::ProgramCounter); return false; }
    const Instruction& ins = code[pc_];
    const OpInfo* info = opcodeInfo(ins.opcode);
    if (!info || !info->executable || (ins.threeOperand && info->form == Form::Divide)) {
        fault(ins.line, std::string("unsupported execution instruction: ") + (info ? info->name : "unknown"), FaultCode::Unsupported);
        return false;
    }
    try {
        if (ins.rd >= 32 || ins.rs >= 32) throw std::out_of_range("invalid register index");
        std::size_t next = pc_ + 1;
        switch (ins.opcode) {
        case Opcode::Nop: break;
        case Opcode::Li: writeRegister(ins.rd, sourceValue(ins.source)); break;
        case Opcode::Move: writeRegister(ins.rd, readReg(ins.rs)); break;
        case Opcode::Mfhi: writeRegister(ins.rd, hi_); break;
        case Opcode::Mflo: writeRegister(ins.rd, lo_); break;
        case Opcode::Lw: writeRegister(ins.rd, memory_.read(addressValue(ins.address), 4)); break;
        case Opcode::Sw: memory_.write(addressValue(ins.address), 4, readReg(ins.rd)); break;
        case Opcode::La: {
            const uint64_t address = addressValue(ins.address);
            if (!memory_.contains(address, 1)) throw AddressError("address out of bounds");
            writeRegister(ins.rd, static_cast<uint32_t>(address));
            break;
        }
        case Opcode::Add: case Opcode::Sub: {
            const int64_t a = signedValue(readReg(ins.rs));
            const int64_t b = signedValue(sourceValue(ins.source));
            const int64_t value = ins.opcode == Opcode::Add ? a + b : a - b;
            if (value < -2147483648LL || value > 2147483647LL) {
                fault(ins.line, "signed arithmetic overflow", FaultCode::Overflow);
                return false;
            }
            writeRegister(ins.rd, static_cast<uint32_t>(value));
            break;
        }
        case Opcode::Addu: writeRegister(ins.rd, readReg(ins.rs) + sourceValue(ins.source)); break;
        case Opcode::Subu: writeRegister(ins.rd, readReg(ins.rs) - sourceValue(ins.source)); break;
        case Opcode::And: writeRegister(ins.rd, readReg(ins.rs) & sourceValue(ins.source)); break;
        case Opcode::Or: writeRegister(ins.rd, readReg(ins.rs) | sourceValue(ins.source)); break;
        case Opcode::Xor: writeRegister(ins.rd, readReg(ins.rs) ^ sourceValue(ins.source)); break;
        case Opcode::Nor: writeRegister(ins.rd, ~(readReg(ins.rs) | sourceValue(ins.source))); break;
        case Opcode::Not: writeRegister(ins.rd, ~sourceValue(ins.source)); break;
        case Opcode::Mult: case Opcode::Multu: {
            const uint32_t a = readReg(ins.rd), b = readReg(ins.rs);
            const uint64_t product = ins.opcode == Opcode::Mult
                ? static_cast<uint64_t>(signedValue(a) * signedValue(b))
                : static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
            lo_ = static_cast<uint32_t>(product);
            hi_ = static_cast<uint32_t>(product >> 32);
            break;
        }
        case Opcode::Div: case Opcode::Divu: {
            const uint32_t a = readReg(ins.rd), b = readReg(ins.rs);
            // Course-undefined division by zero deterministically preserves HI/LO.
            if (b != 0) {
                if (ins.opcode == Opcode::Div) {
                    // int64 avoids host UB for INT32_MIN / -1; low 32 bits are retained.
                    lo_ = static_cast<uint32_t>(signedValue(a) / signedValue(b));
                    hi_ = static_cast<uint32_t>(signedValue(a) % signedValue(b));
                } else { lo_ = a / b; hi_ = a % b; }
            }
            break;
        }
        case Opcode::J:
            if (ins.target >= code.size()) throw std::out_of_range("jump target out of bounds");
            next = ins.target;
            break;
        case Opcode::Beq: case Opcode::Bne: case Opcode::Blt:
        case Opcode::Ble: case Opcode::Bgt: case Opcode::Bge: {
            const uint32_t a = readReg(ins.rs), b = sourceValue(ins.source);
            bool take = false;
            switch (ins.opcode) {
            case Opcode::Beq: take = a == b; break;
            case Opcode::Bne: take = a != b; break;
            case Opcode::Blt: take = signedValue(a) < signedValue(b); break;
            case Opcode::Ble: take = signedValue(a) <= signedValue(b); break;
            case Opcode::Bgt: take = signedValue(a) > signedValue(b); break;
            case Opcode::Bge: take = signedValue(a) >= signedValue(b); break;
            default: break;
            }
            if (take) {
                if (ins.target >= code.size()) throw std::out_of_range("branch target out of bounds");
                next = ins.target;
            }
            break;
        }
        default: fault(ins.line, "unsupported execution instruction", FaultCode::Unsupported); return false;
        }
        pc_ = next;
        ++executed_;
        return true;
    } catch (const MemoryError& error) {
        fault(ins.line, error.what(), FaultCode::Memory);
        diagnostic_.hasAddress = true; diagnostic_.address = error.address; diagnostic_.width = error.width;
        return false;
    } catch (const AddressError& error) {
        fault(ins.line, error.what(), FaultCode::Address);
        return false;
    } catch (const std::out_of_range& error) {
        fault(ins.line, error.what(), FaultCode::Operand);
        return false;
    }
}
}
