#ifndef MIPS_NUMBERS_HPP
#define MIPS_NUMBERS_HPP
#include <cstddef>
#include <cstdint>
#include <string>
namespace mips {
const std::size_t MaxSourceBytes = 4 * 1024 * 1024;
const std::size_t MaxInstructions = 100000;
const std::size_t MaxMemoryBytes = 16 * 1024 * 1024;
const std::size_t DefaultMemoryBytes = 1024;
struct Number {
    int64_t value = 0;
    bool explicitlySigned = false;
};
// Decimal assembly literals: an explicit sign selects the signed range.
bool parseNumber(const std::string& text, Number& result);
bool fits(const Number& number, unsigned bits);
bool parseRegister(const std::string& text, unsigned& result);
bool parseAddress(const std::string& text, uint32_t& result);
bool isIdentifier(const std::string& text);
const char* registerAlias(unsigned index);
int64_t signedValue(uint32_t bits);
std::string hexValue(uint32_t value, unsigned digits = 8);
}
#endif
