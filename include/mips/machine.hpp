#ifndef MIPS_MACHINE_HPP
#define MIPS_MACHINE_HPP
#include "memory.hpp"
#include "diagnostic.hpp"
#include "program.hpp"
#include <array>
#include <memory>
namespace mips {
enum class Status { Simulating, Error };
// Single-threaded machine. Copies share immutable code but own their register/memory state.
// Use ExecutionController rather than sharing a mutable Machine between threads.
class Machine {
public:
    Machine();
    explicit Machine(const Program& program);
    explicit Machine(std::shared_ptr<const Program> program);
    // Public construction defensively copies builders, even shared const views.
    // Subsequent Machine copies share only the internally owned immutable Program.
    bool step();
    void reset();
    const Diagnostic& diagnostic() const { return diagnostic_; }
    void simulation() { step(); }
    uint32_t readReg(unsigned index) const { return registers_.at(index); }
    uint32_t readPC() const { return static_cast<uint32_t>(pc_); }
    uint32_t readHI() const { return hi_; }
    uint32_t readLO() const { return lo_; }
    uint32_t readMEM(uint64_t address, unsigned width) const { return memory_.read(address, width); }
    std::size_t memSize() const { return memory_.size(); }
    const std::vector<uint8_t>& memoryBytes() const { return memory_.bytes(); }
    Status getStatus() const { return status_; }
    const std::string& error() const { return error_; }
    uint64_t executedSteps() const { return executed_; }
    const std::vector<Instruction>& getInstrVector() const;
    const Instruction& getInstruction(std::size_t index) const { return getInstrVector().at(index); }
    const std::shared_ptr<const Program>& program() const { return program_; }
    bool B_labelExist(const std::string& name) const;
    bool D_labelExist(const std::string& name) const;
private:
    void fault(std::size_t line, const std::string& message, FaultCode code = FaultCode::Operand);
    void initialize();
    uint32_t sourceValue(const Source& source) const;
    uint64_t addressValue(const MemoryRef& address) const;
    void writeRegister(unsigned index, uint32_t value);
    std::shared_ptr<const Program> program_;
    Memory memory_;
    std::array<uint32_t, 32> registers_{};
    std::size_t pc_ = 0;
    uint32_t hi_ = 0, lo_ = 0;
    uint64_t executed_ = 0;
    Status status_ = Status::Error;
    std::string error_ = "Error:1: no program loaded";
    Diagnostic diagnostic_;
};
}
#endif
