#include "parser.hpp"
#include <stdexcept>
#include <utility>
namespace {
using namespace mips;
using Line = std::vector<Token>;
struct ParseError : std::runtime_error {
    std::size_t line;
    ParseError(std::size_t where, const std::string& what) : std::runtime_error(what), line(where) {}
};
struct Cursor {
    const Line& tokens;
    std::size_t pos;
    Cursor(const Line& line, std::size_t start = 0) : tokens(line), pos(start) {}
    std::size_t line() const { return tokens.empty() ? 1 : tokens.front().line(); }
    bool end() const { return pos == tokens.size(); }
    bool is(TokenType type) const { return !end() && tokens[pos].type() == type; }
    void fail(const std::string& message) const { throw ParseError(line(), message); }
    void expect(TokenType type, const char* message) {
        if (!is(type)) fail(message);
        ++pos;
    }
    std::string word() {
        if (!is(STRING)) fail("expected an operand or identifier");
        return tokens[pos++].value();
    }
    void comma() { expect(SEP, "expected ',' between operands"); }
    void finish() const { if (!end()) fail("unexpected trailing token"); }
};
struct Symbol {
    enum class Kind { Constant, Data, Text };
    Kind kind = Kind::Constant;
    Number number;
    std::size_t address = 0;
};
struct Pending { Line tokens; std::size_t start; };
class Assembler {
public:
    explicit Assembler(std::size_t size) : capacity_(size) {}
    Program assemble(const TokenList& tokens) {
        Line row;
        for (const Token& token : tokens) {
            if (token.type() == ERROR) {
                std::string message = token.value();
                if (message.compare(0, 7, "Error: ") == 0) message.erase(0, 7);
                throw ParseError(token.line(), message);
            }
            if (token.type() == EOL) { if (!row.empty()) firstPass(row); row.clear(); }
            else row.push_back(token);
        }
        if (!row.empty()) firstPass(row);
        program_.instructions.reserve(pending_.size());
        for (const Pending& pending : pending_) program_.instructions.push_back(instruction(pending));
        program_.initialMemory.resize(capacity_, 0);
        const auto main = program_.textLabels.find("main");
        if (main != program_.textLabels.end()) { program_.hasEntry = true; program_.entry = main->second; }
        return std::move(program_);
    }
private:
    enum class Section { None, Data, Text };
    Section section_ = Section::None;
    bool sawData_ = false, sawText_ = false;
    std::size_t capacity_;
    Program program_;
    std::unordered_map<std::string, Symbol> symbols_;
    std::vector<Pending> pending_;
    void addSymbol(Cursor& c, const std::string& name, const Symbol& symbol) {
        if (!isIdentifier(name)) c.fail("invalid identifier: " + name);
        if (!symbols_.emplace(name, symbol).second) c.fail("duplicate symbol: " + name);
    }
    Number number(Cursor& c, const std::string& word) const {
        const auto found = symbols_.find(word);
        if (found != symbols_.end() && found->second.kind == Symbol::Kind::Constant) return found->second.number;
        Number n;
        if (!parseNumber(word, n)) c.fail("invalid, out-of-range or undefined integer: " + word);
        return n;
    }
    void reserveBytes(Cursor& c, std::size_t count) const {
        if (count > capacity_ - program_.initialMemory.size()) c.fail("data exceeds configured memory");
    }
    void firstPass(const Line& row) {
        Cursor c(row);
        std::string first = c.word();
        if (first == ".data" || first == ".text") {
            c.finish();
            if (first == ".data") {
                if (sawData_ || sawText_) c.fail(".data must appear once, before .text");
                sawData_ = true; section_ = Section::Data;
            } else {
                if (sawText_) c.fail("duplicate .text section");
                sawText_ = true; section_ = Section::Text;
            }
            return;
        }
        if (section_ == Section::None) c.fail("expected .data or .text section");
        if (!first.empty() && first.back() == ':') {
            const std::string name = first.substr(0, first.size() - 1);
            Symbol symbol;
            symbol.kind = section_ == Section::Data ? Symbol::Kind::Data : Symbol::Kind::Text;
            symbol.address = section_ == Section::Data ? program_.initialMemory.size() : pending_.size();
            addSymbol(c, name, symbol);
            if (section_ == Section::Data) program_.dataLabels.emplace(name, symbol.address);
            else program_.textLabels.emplace(name, symbol.address);
            if (c.end()) return;
            first = c.word();
        }
        if (section_ == Section::Text) {
            if (!findOpcode(first)) c.fail("unknown instruction: " + first);
            if (pending_.size() >= MaxInstructions) c.fail("instruction count exceeds 100000");
            pending_.push_back(Pending{row, c.pos - 1});
            return;
        }
        if (c.is(EQUAL)) {
            c.expect(EQUAL, "expected '='");
            const Number n = number(c, c.word());
            c.finish();
            Symbol symbol; symbol.number = n;
            addSymbol(c, first, symbol);
            return;
        }
        if (first == ".ascii" || first == ".asciiz") {
            c.expect(STRING_DELIM, "expected opening quote");
            const std::string value = c.word();
            c.expect(STRING_DELIM, "expected closing quote");
            c.finish();
            reserveBytes(c, value.size() + (first == ".asciiz" ? 1 : 0));
            program_.initialMemory.insert(program_.initialMemory.end(), value.begin(), value.end());
            if (first == ".asciiz") program_.initialMemory.push_back(0);
            return;
        }
        const unsigned width = first == ".word" ? 4 : (first == ".half" ? 2 : (first == ".byte" ? 1 : 0));
        if (width == 0 && first != ".space") c.fail("unknown data directive: " + first);
        do {
            const Number n = number(c, c.word());
            if (first == ".space") {
                if (n.value < 0) c.fail(".space size must not be negative");
                const std::size_t size = static_cast<std::size_t>(n.value);
                reserveBytes(c, size);
                program_.initialMemory.resize(program_.initialMemory.size() + size, 0);
            } else {
                if (!fits(n, width * 8)) c.fail("integer does not fit data width");
                reserveBytes(c, width);
                const uint32_t bits = static_cast<uint32_t>(n.value);
                for (unsigned i = 0; i < width; ++i)
                    program_.initialMemory.push_back(static_cast<uint8_t>(bits >> (8 * i)));
            }
            if (c.end()) break;
            c.comma();
        } while (true);
    }
    unsigned reg(Cursor& c) const {
        const std::string word = c.word();
        unsigned index;
        if (!parseRegister(word, index)) c.fail("invalid register: " + word);
        return index;
    }
    Source source(Cursor& c) const {
        const std::string word = c.word();
        if (!word.empty() && word[0] == '$') {
            unsigned index;
            if (!parseRegister(word, index)) c.fail("invalid register: " + word);
            return Source::reg(index);
        }
        return Source::immediate(static_cast<uint32_t>(number(c, word).value));
    }
    Source base(Cursor& c, const std::string& word) const {
        if (!word.empty() && word[0] == '$') {
            unsigned index;
            if (!parseRegister(word, index)) c.fail("invalid register: " + word);
            return Source::reg(index);
        }
        const auto found = symbols_.find(word);
        if (found != symbols_.end() && found->second.kind == Symbol::Kind::Data)
            return Source::immediate(static_cast<uint32_t>(found->second.address));
        return Source::immediate(static_cast<uint32_t>(number(c, word).value));
    }
    MemoryRef memoryRef(Cursor& c) const {
        MemoryRef ref;
        if (!c.is(OPEN_PAREN)) {
            const std::string word = c.word();
            if (!c.is(OPEN_PAREN)) { ref.base = base(c, word); return ref; }
            ref.offset = number(c, word).value;
            if (ref.offset < -2147483648LL || ref.offset > 2147483647LL) c.fail("offset exceeds signed 32-bit range");
        }
        c.expect(OPEN_PAREN, "expected '('");
        const std::string word = c.word();
        ref.base = base(c, word);
        c.expect(CLOSE_PAREN, "expected ')'");
        return ref;
    }
    std::size_t target(Cursor& c) const {
        const std::string word = c.word();
        const auto found = program_.textLabels.find(word);
        if (found == program_.textLabels.end()) c.fail("undefined instruction label: " + word);
        return found->second;
    }
    Instruction instruction(const Pending& pending) const {
        Cursor c(pending.tokens, pending.start);
        const OpInfo& op = *findOpcode(c.word()); // first pass validated the mnemonic.
        Instruction ins; ins.opcode = op.opcode; ins.line = c.line();
        switch (op.form) {
        case Form::None: break;
        case Form::Memory: ins.rd = reg(c); c.comma(); ins.address = memoryRef(c); break;
        case Form::Immediate:
            ins.rd = reg(c); c.comma(); ins.source = Source::immediate(static_cast<uint32_t>(number(c, c.word()).value)); break;
        case Form::SingleRegister: ins.rd = reg(c); break;
        case Form::RegisterPair: ins.rd = reg(c); c.comma(); ins.rs = reg(c); break;
        case Form::RegRegSource:
            ins.rd = reg(c); c.comma(); ins.rs = reg(c); c.comma(); ins.source = source(c); break;
        case Form::Divide:
            ins.rd = reg(c); c.comma(); ins.rs = reg(c);
            if (!c.end()) { c.comma(); ins.source = source(c); ins.threeOperand = true; }
            break;
        case Form::Source: ins.rd = reg(c); c.comma(); ins.source = source(c); break;
        case Form::Branch: ins.rs = reg(c); c.comma(); ins.source = source(c); c.comma(); ins.target = target(c); break;
        case Form::Jump: ins.target = target(c); break;
        }
        c.finish();
        return ins;
    }
};
}
Parse::Parse(std::size_t memoryBytes) : memoryBytes_(memoryBytes) {
    if (memoryBytes == 0 || memoryBytes > mips::MaxMemoryBytes) throw std::invalid_argument("invalid memory capacity");
}
bool Parse::parse(const TokenList& tokens) {
    program_.reset(); error_.clear(); line_ = 1;
    try {
        std::size_t size = 0;
        for (const Token& token : tokens) {
            line_ = token.line();
            if (token.value().size() > mips::MaxSourceBytes - size) throw ParseError(line_, "token input too large");
            size += token.value().size();
        }
        if (tokens.size() > mips::MaxSourceBytes) throw ParseError(line_, "too many tokens");
        Assembler assembler(memoryBytes_);
        program_ = std::make_shared<const mips::Program>(assembler.assemble(tokens));
        return true;
    } catch (const ParseError& e) {
        line_ = e.line;
        error_ = "Error:" + std::to_string(line_) + ": " + e.what();
        return false;
    }
}
VirtualMachine Parse::getVM() const { return VirtualMachine(program_); }
std::size_t Parse::getMainLine() const {
    return program_ && program_->hasEntry && program_->entry < program_->instructions.size()
        ? program_->instructions[program_->entry].line : 0;
}
