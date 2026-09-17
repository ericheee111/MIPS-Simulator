#include "mips/numbers.hpp"
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace mips {
namespace {
const char* const aliases[] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};
bool alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool unsignedValue(const std::string& text, std::size_t pos, unsigned base,
                   uint64_t limit, uint64_t& result) {
    if (pos == text.size()) return false;
    uint64_t value = 0;
    for (; pos < text.size(); ++pos) {
        const char c = text[pos];
        unsigned digit;
        if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
        else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(c - 'A' + 10);
        else return false;
        if (digit >= base || digit > limit || value > (limit - digit) / base) return false;
        value = value * base + digit;
    }
    result = value;
    return true;
}
}
bool parseNumber(const std::string& text, Number& result) {
    if (text.empty()) return false;
    const bool sign = text[0] == '+' || text[0] == '-';
    const bool negative = text[0] == '-';
    const uint64_t limit = negative ? 2147483648ULL : (sign ? 2147483647ULL : 4294967295ULL);
    uint64_t value;
    if (!unsignedValue(text, sign ? 1 : 0, 10, limit, value)) return false;
    result.value = negative ? -static_cast<int64_t>(value) : static_cast<int64_t>(value);
    result.explicitlySigned = sign;
    return true;
}
bool fits(const Number& n, unsigned bits) {
    if (bits == 0 || bits > 32) return false;
    const int64_t maximum = (int64_t(1) << (n.explicitlySigned ? bits - 1 : bits)) - 1;
    const int64_t minimum = n.explicitlySigned ? -(int64_t(1) << (bits - 1)) : 0;
    return n.value >= minimum && n.value <= maximum;
}
bool parseRegister(const std::string& text, unsigned& result) {
    if (text.size() < 2 || text[0] != '$') return false;
    uint64_t value;
    if (unsignedValue(text, 1, 10, 31, value)) {
        result = static_cast<unsigned>(value);
        return true;
    }
    for (unsigned i = 0; i < 32; ++i) {
        if (text.compare(1, std::string::npos, aliases[i]) == 0) {
            result = i;
            return true;
        }
    }
    return false;
}
bool parseAddress(const std::string& text, uint32_t& result) {
    const bool hex = text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
    uint64_t value;
    if (!unsignedValue(text, hex ? 2 : 0, hex ? 16 : 10, 4294967295ULL, value)) return false;
    result = static_cast<uint32_t>(value);
    return true;
}
bool isIdentifier(const std::string& text) {
    if (text.empty() || !alpha(text[0])) return false;
    for (char c : text) if (!alpha(c) && !(c >= '0' && c <= '9')) return false;
    return true;
}
const char* registerAlias(unsigned index) {
    if (index >= 32) throw std::out_of_range("invalid register index");
    return aliases[index];
}
int64_t signedValue(uint32_t bits) {
    return bits <= 0x7fffffffU ? static_cast<int64_t>(bits) : static_cast<int64_t>(bits) - 4294967296LL;
}
std::string hexValue(uint32_t value, unsigned digits) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setfill('0') << std::setw(static_cast<int>(digits)) << value;
    return out.str();
}
}
