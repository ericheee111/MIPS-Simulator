#ifndef PARSER_HPP
#define PARSER_HPP
#include "token.hpp"
#include "mips/machine.hpp"
// Compatibility names retained for the original educational tests/frontends.
using VirtualMachine = mips::Machine;
using VM_Status = mips::Status;
using Instruction = mips::Instruction;
class Parse {
public:
    explicit Parse(std::size_t memoryBytes = mips::DefaultMemoryBytes);
    // Syntax and configured input-limit failures return false with error().
    // Host allocation failures may throw (including while formatting an error).
    // Each attempt clears the previous program; no partial candidate is published.
    bool parse(const TokenList& tokens);
    VirtualMachine getVM() const;
    std::size_t getLine() const { return line_; }
    std::size_t getMainLine() const;
    const std::string& error() const { return error_; }
    const std::shared_ptr<const mips::Program>& program() const { return program_; }
private:
    std::size_t memoryBytes_;
    std::size_t line_ = 1;
    std::string error_;
    std::shared_ptr<const mips::Program> program_;
};
#endif
