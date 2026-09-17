#include "mips/memory.hpp"
#include <stdexcept>
namespace mips {
namespace {
std::size_t checkedSize(std::size_t size) {
    if (size == 0 || size > MaxMemoryBytes) throw std::invalid_argument("memory size must be 1..16777216 bytes");
    return size;
}
}
Memory::Memory(std::size_t size) : bytes_(checkedSize(size), 0) {}
Memory::Memory(const std::vector<uint8_t>& bytes) : Memory(bytes.size()) { bytes_ = bytes; }
bool Memory::contains(uint64_t address, unsigned width) const {
    return (width == 1 || width == 2 || width == 4) &&
        width <= bytes_.size() && address <= bytes_.size() - width;
}
uint32_t Memory::read(uint64_t address, unsigned width) const {
    if (!contains(address, width)) throw MemoryError(address, width);
    uint32_t value = 0;
    for (unsigned i = 0; i < width; ++i)
        value |= uint32_t(bytes_[static_cast<std::size_t>(address) + i]) << (8 * i);
    return value;
}
void Memory::write(uint64_t address, unsigned width, uint32_t value) {
    // Check the entire range before touching any byte: faults cannot partly store a word.
    if (!contains(address, width)) throw MemoryError(address, width);
    for (unsigned i = 0; i < width; ++i)
        bytes_[static_cast<std::size_t>(address) + i] = static_cast<uint8_t>(value >> (8 * i));
}
}
