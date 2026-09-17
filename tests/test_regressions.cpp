#include "catch.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "mips/numbers.hpp"
#include <random>
#include <sstream>
#include <stdexcept>
using namespace mips;
namespace {
TokenList lex(const std::string& text) { std::istringstream stream(text); return tokenize(stream); }
Machine assemble(const std::string& text, std::size_t memory = DefaultMemoryBytes) {
    Parse parser(memory);
    if (!parser.parse(lex(text))) throw std::runtime_error(parser.error());
    return parser.getVM();
}
Machine program(const std::string& instructions) { return assemble(".text\nmain:\n" + instructions); }
void steps(Machine& machine, unsigned count) {
    for (unsigned i = 0; i < count; ++i) { INFO(machine.error()); REQUIRE(machine.step()); }
}
}
TEST_CASE("comments preserve tokens and source locations", "[regression][lexer]") {
    const auto tokens = lex("# heading\n.text\nmain:\nli $t0, 7# adjacent\nli $t1, 8\n");
    std::vector<Token> values(tokens.begin(), tokens.end());
    REQUIRE(values.front().line() == 2);
    auto machine = assemble("# heading\n.text\nmain:\nli $t0, 7# adjacent\nli $t1, 8\n");
    REQUIRE(machine.getInstruction(0).line == 4);
    REQUIRE(machine.getInstruction(1).line == 5);
    steps(machine, 2);
    REQUIRE(machine.readReg(8) == 7);
    REQUIRE(machine.readReg(9) == 8);
}
TEST_CASE("EOF whitespace has identical meaning", "[regression][lexer]") {
    const std::string source = ".text\nmain:\nli $t0, 7";
    const auto plain = lex(source);
    for (const std::string suffix : {" ", "\t\r", "\n", " # comment"}) {
        INFO(suffix);
        REQUIRE(lex(source + suffix) == plain);
    }
}
TEST_CASE("lexer rejects incomplete or invalid input", "[regression][lexer]") {
    for (const std::string source : {".ascii \"ABC", "lw $t0, 4($t1", "x)", "((x))", "\"a\nb\""}) {
        const auto tokens = lex(source);
        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokens.back().type() == ERROR);
    }
    REQUIRE(lex(std::string(1, '\0')).back().type() == ERROR);
    REQUIRE(lex(std::string(MaxSourceBytes + 1, ' ')).back().type() == ERROR);
    auto empty = lex("\"\"");
    REQUIRE(empty.size() == 4); // delimiters, empty value, EOL
    REQUIRE(std::next(empty.begin())->value().empty());
    std::istringstream broken; broken.setstate(std::ios::badbit);
    REQUIRE(tokenize(broken).back().type() == ERROR);
}
TEST_CASE("signed and full width immediates are values not symbol names", "[regression][parser]") {
    auto machine = program("li $t0, -1\nli $t1, +7\nli $t2, 4294967295\n");
    steps(machine, 3);
    REQUIRE(machine.readReg(8) == UINT32_MAX);
    REQUIRE(machine.readReg(9) == 7);
    REQUIRE(machine.readReg(10) == UINT32_MAX);
}
TEST_CASE("numbers reject malformed and out of range input without exceptions", "[numbers]") {
    for (const std::string text : {"", "+", "-", "4294967296", "+2147483648", "-2147483649", "1x", "0x10", "999999999999999999999999"}) {
        Number n;
        REQUIRE_FALSE(parseNumber(text, n));
    }
    unsigned index = 99;
    for (const std::string text : {"$", "$32", "$-1", "$99999999999999999999", "$bad", ""}) REQUIRE_FALSE(parseRegister(text, index));
    for (unsigned i = 0; i < 32; ++i) {
        REQUIRE(parseRegister("$" + std::to_string(i), index)); REQUIRE(index == i);
        REQUIRE(parseRegister("$" + std::string(registerAlias(i)), index)); REQUIRE(index == i);
    }
    uint32_t address;
    REQUIRE(parseAddress("0xffffffff", address)); REQUIRE(address == UINT32_MAX);
    for (const std::string text : {"0x", "-1", "0x100000000", "", "99999999999999999999"}) REQUIRE_FALSE(parseAddress(text, address));
}
TEST_CASE("strings are copied completely with explicit NUL termination", "[regression][parser]") {
    auto machine = assemble(".data\ns: .asciiz \"ABC\"\nx: .byte 9\n");
    const std::vector<uint8_t> expected{65,66,67,0,9};
    REQUIRE(std::equal(expected.begin(), expected.end(), machine.memoryBytes().begin()));
    for (const std::string text : {"add", "123", "$t0", "name:", "", "#(,)="}) {
        auto value = assemble(".data\ns: .ascii \"" + text + "\"\nx: .byte 9\n");
        REQUIRE(value.readMEM(text.size(), 1) == 9);
        for (std::size_t i = 0; i < text.size(); ++i) REQUIRE(value.readMEM(i, 1) == static_cast<uint8_t>(text[i]));
    }
}
TEST_CASE("parser rejects partial statements and invalid symbols", "[regression][parser]") {
    for (const std::string source : {
        ".data\nx =", ".data\nx: .word", ".data\nx: .space -1", ".text\nmain:\nli $t0, ",
        ".text\nmain:\nli $, 1", ".text\nmain:\nli $0, +", ".text\nmain:\nli $t0, 4294967296",
        ".data\nx: .word 1\nx: .word 2", ".text\nmain:\nj missing", ".text\nmain:\nmain:\nnop",
        ".data\n.word 1,", ".text\nmain:\nli $t0, 7 nop", ".text\nmain:\nunknown", ".data\n.word .text"}) {
        INFO(source); Parse parser;
        REQUIRE_FALSE(parser.parse(lex(source)));
        REQUIRE(parser.error().find("Error:") == 0);
        REQUIRE_FALSE(parser.program());
        REQUIRE(parser.getVM().getStatus() == Status::Error);
    }
}
TEST_CASE("symbol signedness is not inherited from the previous literal", "[regression][parser]") {
    auto machine = assemble(".data\nNEG = -1\nPOS = 1\nx: .word NEG\n");
    REQUIRE(machine.readMEM(0,4) == UINT32_MAX);
    Parse parser;
    REQUIRE_FALSE(parser.parse(lex(".data\nSIGNED = +250\n.byte SIGNED")));
    REQUIRE(parser.parse(lex(".data\nUNSIGNED = 250\n.byte UNSIGNED")));
}
TEST_CASE("failed parse and repeated parse never retain old machine data", "[parser]") {
    Parse parser;
    REQUIRE(parser.parse(lex(".text\nmain:\nli $t0, 4")));
    auto old = parser.getVM();
    REQUIRE_FALSE(parser.parse(lex(".text\nmain:\nli $t0,")));
    REQUIRE_FALSE(parser.program());
    REQUIRE(parser.parse(lex(".text\nmain:\nli $t0, 9")));
    auto fresh = parser.getVM();
    steps(old,1); steps(fresh,1);
    REQUIRE(old.readReg(8) == 4); REQUIRE(fresh.readReg(8) == 9);
}
TEST_CASE("main can be the first or a later instruction", "[regression][machine]") {
    auto machine = assemble(".text\nli $t0, 99\nmain:\nli $t0, 7\n");
    REQUIRE(machine.readPC() == 1); steps(machine,1); REQUIRE(machine.readReg(8) == 7);
    REQUIRE(assemble(".text\nnop").getStatus() == Status::Error);
    REQUIRE(assemble(".text\nmain:\n").getStatus() == Status::Error);
}
TEST_CASE("configured memory capacity is bounded during assembly", "[parser][memory]") {
    Parse parser(8);
    REQUIRE(parser.parse(lex(".data\n.space 8")));
    REQUIRE(parser.getVM().memSize() == 8);
    for (const std::string source : {".data\n.space 9", ".data\n.space 4294967295", ".data\n.word 1,2,3", ".data\n.asciiz \"12345678\""})
        REQUIRE_FALSE(parser.parse(lex(source)));
    REQUIRE_THROWS_AS(Parse(0), std::invalid_argument);
    REQUIRE_THROWS_AS(Parse(MaxMemoryBytes+1), std::invalid_argument);
    REQUIRE_THROWS_AS(Memory(0), std::invalid_argument);
}
TEST_CASE("checked memory writes validate the whole range before any side effect", "[memory]") {
    Memory memory(8);
    memory.write(1,4,0x87654321); // intentionally unaligned
    REQUIRE(memory.read(1,4) == 0x87654321);
    REQUIRE(memory.read(1,1) == 0x21);
    REQUIRE(memory.read(2,2) == 0x6543);
    const auto before = memory.bytes();
    for (uint64_t address : std::vector<uint64_t>{5, 8, 65536, UINT64_MAX}) {
        REQUIRE_THROWS_AS(memory.write(address,4,0), std::out_of_range);
        REQUIRE_THROWS_AS(memory.read(address,4), std::out_of_range);
        REQUIRE(memory.bytes() == before);
    }
    REQUIRE_THROWS_AS(memory.read(0,0), std::out_of_range);
    REQUIRE_THROWS_AS(memory.write(0,3,0), std::out_of_range);
}
TEST_CASE("zero register ignores writes but not load faults", "[regression][machine]") {
    auto machine = program("li $zero, 7\naddu $0, $0, 9\nlw $0, 1024");
    steps(machine,2); REQUIRE(machine.readReg(0) == 0);
    REQUIRE_FALSE(machine.step()); REQUIRE(machine.getStatus() == Status::Error);
    REQUIRE(machine.readPC() == 2);
}
TEST_CASE("load and store support absolute and negative-offset addresses", "[regression][machine]") {
    auto machine = assemble(".data\nx: .word 11,22\n.text\nmain:\nlw $t0, 0\nli $t1, 4\nlw $t2, -4($t1)\nli $t3, 99\nsw $t3, -4($t1)\n");
    steps(machine,5);
    REQUIRE(machine.readReg(8) == 11); REQUIRE(machine.readReg(10) == 11);
    REQUIRE(machine.readMEM(0,4) == 99);
}
TEST_CASE("out-of-range accesses are errors with no truncation or partial writes", "[regression][machine]") {
    for (const std::string instruction : {"lw $t1, 1021", "sw $t0, 1021", "lw $t1, 65536", "lw $t1, -1($zero)", "sw $t0, -1($zero)", "la $t0, 1024"}) {
        INFO(instruction);
        auto machine = program("li $t0, 7\n" + instruction);
        steps(machine,1); const auto before = machine.memoryBytes();
        REQUIRE_FALSE(machine.step()); REQUIRE(machine.readPC() == 1);
        REQUIRE(machine.readReg(8) == 7); REQUIRE(machine.memoryBytes() == before);
        REQUIRE(machine.getStatus() == Status::Error);
        REQUIRE_FALSE(machine.step()); REQUIRE(machine.readPC() == 1);
    }
}
TEST_CASE("signed overflow is sticky and leaves destination unchanged", "[regression][machine]") {
    for (const std::string operation : {"li $t0, 2147483647\nadd $t2, $t0, 1", "li $t0, -2147483648\nsub $t2, $t0, 1"}) {
        auto machine = program("li $t2, 42\n" + operation);
        steps(machine,2); REQUIRE_FALSE(machine.step());
        REQUIRE(machine.readReg(10) == 42); REQUIRE(machine.readPC() == 2);
        REQUIRE(machine.error().find("overflow") != std::string::npos);
        REQUIRE_FALSE(machine.step()); REQUIRE(machine.executedSteps() == 2);
    }
}
TEST_CASE("unsigned arithmetic wraps and widens correctly", "[regression][machine]") {
    auto machine = program("li $t0, 4294967295\naddu $t1, $t0, 1\nsubu $t2, $zero, 1\nli $t3, 2\ndivu $t0, $t3\n");
    steps(machine,5);
    REQUIRE(machine.readReg(9) == 0); REQUIRE(machine.readReg(10) == UINT32_MAX);
    REQUIRE(machine.readLO() == 2147483647); REQUIRE(machine.readHI() == 1);
    auto multiply = program("li $t0, 65536\nmultu $t0, $t0");
    steps(multiply,2); REQUIRE(multiply.readLO() == 0); REQUIRE(multiply.readHI() == 1);
}
TEST_CASE("signed division corner cases never invoke host integer UB", "[machine]") {
    auto machine = program("li $t0, -2147483648\nli $t1, -1\ndiv $t0, $t1\ndiv $t0, $zero");
    steps(machine,3); REQUIRE(machine.readLO() == 0x80000000); REQUIRE(machine.readHI() == 0);
    steps(machine,1); REQUIRE(machine.readLO() == 0x80000000); REQUIRE(machine.readHI() == 0);
}
TEST_CASE("branch sources can be immediates and comparisons are signed", "[regression][machine]") {
    for (const std::string condition : {"beq $t0, -1", "bne $t0, 1", "blt $t0, 1", "ble $t0, -1", "bgt $zero, $t0", "bge $zero, 0"}) {
        auto machine = program("li $t0, -1\n" + condition + ", done\nli $t1, 99\ndone:\nnop");
        steps(machine,2); REQUIRE(machine.readPC() == 3); REQUIRE(machine.readReg(9) == 0);
    }
}
TEST_CASE("parse-only opcodes fail explicitly during execution", "[machine]") {
    for (const auto& op : instructionSet()) {
        if (op.executable) continue;
        std::string text(op.name);
        if (op.form == Form::Memory) text += " $t0, 0";
        else if (op.form == Form::SingleRegister) text += " $t0";
        else if (op.form == Form::RegisterPair) text += " $t0, $t1";
        else if (op.form == Form::RegRegSource) text += " $t0, $t1, 1";
        auto machine = program(text);
        REQUIRE_FALSE(machine.step()); REQUIRE(machine.readPC() == 0);
        REQUIRE(machine.error().find("unsupported") != std::string::npos);
    }
    auto divide = program("div $t0, $t1, $t2");
    REQUIRE_FALSE(divide.step()); REQUIRE(divide.readPC() == 0);
}
TEST_CASE("past end and invalid branch target report errors", "[machine]") {
    auto machine = program("nop"); steps(machine,1);
    REQUIRE_FALSE(machine.step()); REQUIRE(machine.readPC() == 1);
    auto jump = program("j done\ndone:"); REQUIRE_FALSE(jump.step()); REQUIRE(jump.readPC() == 0);
}
TEST_CASE("machine copies isolate state and share immutable code", "[machine]") {
    auto first = program("li $t0, 7\nsw $t0, 0"); auto second = first;
    REQUIRE(first.program() == second.program()); steps(first,2);
    REQUIRE(second.readPC() == 0); REQUIRE(second.readReg(8) == 0); REQUIRE(second.readMEM(0,4) == 0);
    REQUIRE_THROWS_AS(second.readReg(32), std::out_of_range);
    REQUIRE_THROWS_AS(second.getInstruction(2), std::out_of_range);
}
TEST_CASE("public IR invalid operands fail before mutations", "[machine]") {
    auto seed = program("sw $zero, 0");
    for (unsigned scenario = 0; scenario < 4; ++scenario) {
        Program raw = *seed.program();
        auto& instruction = raw.instructions[0];
        if (scenario == 0) instruction.rd = 32;
        if (scenario == 1) instruction.address.base = Source::reg(99);
        if (scenario == 2) instruction.address.offset = INT64_MAX;
        if (scenario == 3) instruction.opcode = static_cast<Opcode>(999);
        Machine machine(raw);
        REQUIRE_FALSE(machine.step()); REQUIRE(machine.readPC() == 0); REQUIRE(machine.readMEM(0,4) == 0);
    }
}
TEST_CASE("fixed-seed arithmetic oracle exercises high unsigned operands", "[property][machine]") {
    std::mt19937 generator(20260917);
    for (unsigned i = 0; i < 200; ++i) {
        const uint32_t a = generator(), b = generator();
        auto machine = program("li $t0, " + std::to_string(a) + "\nli $t1, " + std::to_string(b) +
            "\naddu $t2, $t0, $t1\nsubu $t3, $t0, $t1\nmultu $t0, $t1\n");
        steps(machine,5);
        const uint64_t product = static_cast<uint64_t>(a) * b;
        REQUIRE(machine.readReg(10) == static_cast<uint32_t>(static_cast<uint64_t>(a) + b));
        REQUIRE(machine.readReg(11) == static_cast<uint32_t>(static_cast<uint64_t>(a) - b));
        REQUIRE(machine.readLO() == static_cast<uint32_t>(product));
        REQUIRE(machine.readHI() == product >> 32);
    }
}
TEST_CASE("bounded malformed-input corpus must not escape parsing", "[property][parser]") {
    std::mt19937 generator(17);
    for (unsigned i = 0; i < 1000; ++i) {
        std::string source;
        for (unsigned j = 0, length = generator() % 128; j < length; ++j) source += static_cast<char>(generator() % 256);
        Parse parser;
        REQUIRE_NOTHROW(parser.parse(lex(source)));
    }
}
