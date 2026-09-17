#ifndef MIPS_MEMORY_HPP
#define MIPS_MEMORY_HPP
#include "numbers.hpp"
#include <vector>
namespace mips {
class Memory {
public:
    explicit Memory(std::size_t size = DefaultMemoryBytes);
    explicit Memory(const std::vector<uint8_t>& bytes);
    uint32_t read(uint64_t address, unsigned width) const;
    void write(uint64_t address, unsigned width, uint32_t value);
    bool contains(uint64_t address, unsigned width) const;
    std::size_t size() const { return bytes_.size(); }
    const std::vector<uint8_t>& bytes() const { return bytes_; }
private:
    std::vector<uint8_t> bytes_;
};
}
#endif
