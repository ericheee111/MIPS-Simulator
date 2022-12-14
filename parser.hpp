#ifndef PARSER_HPP
#define PARSER_HPP

#include "token.hpp"
#include <vector>
#include <cstdint>
#include <cassert>
#include <set>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <limits>

enum Register {
	R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R16,
	R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31, errors, empty
};

enum class VM_Status {
	Simulating, Error
};

static std::unordered_map<Register, int> regi{ {R0, 0}, {R1, 1}, {R2, 2}, {R3, 3}, {R4, 4}, {R5, 5}, {R6, 6}, {R7, 7},
											   {R8, 8}, {R9, 9}, {R10, 10}, {R11, 11}, {R12, 12}, {R13, 13}, {R14, 14}, {R15, 15},
											   {R16, 16}, {R17, 17}, {R18, 18}, {R19, 19}, {R20, 20}, {R21, 21}, {R22, 22}, {R23, 23},
											   {R24, 24}, {R25, 25}, {R26, 26}, {R27, 27}, {R28, 28}, {R29, 29}, {R30, 30}, {R31, 31} };


class Instruction {
public:
	void setOp(std::string str);
	void setRS(int irs);
	void setRT(int irt);
	void setRD(int ird);
	void setImm(long int x);

	// call this to if have a offset, to change the offset to true, indicates this instruction has a offset
	void setOffset(bool x);
	void setMemref(int x);
	void setmemsize(int x);
	void setLine(std::size_t lineNum);
	Register findReg(int reg);
	std::string readOP();
	Register readRS();
	Register readRT();
	Register readRD();
	uint32_t readIMM();
	uint32_t readMEMREF();
	int readLineNum();
	int getMemSize();
	bool readOffset();
	bool readUMem();
	bool readURS();
	bool readURT();
	bool readURD();
	bool readUIMM();

private:
	std::string opcode;
	Register rs = empty;
	Register rt = empty;
	Register rd = empty;
	long int immediate = 0;
	uint16_t memref = 0;
	int memsize = 0;
	int line_number = 0;

	bool offset = false;		// indicates if has offset
	bool urs = false;
	bool urt = false;
	bool urd = false;
	bool uimm = false;
	bool umemref = false;
};

class VirtualMachine {
public:
	void addBLabels(std::string str, long int i);		// add branch labels
	void addDLabels(std::string str);		// add data labels
	void addDLType(std::string str, std::string type);		// add data labels
	void addConsts(std::string str, long int i);
	bool D_labelExist(std::string str);
	bool B_labelExist(std::string str);
	bool constExist(std::string str);
	long int readConst(std::string str);
	long int readDLabel(std::string str);
	std::string readDLType(std::string str);
	long int readBLabel(std::string str);
	void pushMem(long int values, int size);
	void pushMemSpace(int size);
	void pushStrMem(std::string str);
	void pushInstruction(Instruction instr);
	uint16_t getInstrIndex();
	void incre_ins_index();
	int readInsIndex();
	uint32_t readMEM(uint16_t address, int size);
	void fillMEM();
	void simulation();				// simulating the program
	void calculation(Instruction ins, VM_Status status);
	int memSize();
	uint32_t readReg(int i);
	uint32_t readPC();
	uint32_t readHI();
	uint32_t readLO();
	VM_Status getStatus();
	Instruction getInstruction(int index);
	std::vector<Instruction> getInstrVector();
private:
	size_t pc = 0;
	uint32_t hi = 0;
	uint32_t lo = 0;
	uint16_t byteCounter = 0;			// counting the memory address of the data labels
	uint16_t instru_index = 0;			// counting instructino index for branch labels
	std::array<uint32_t, 32> reg = { 0 };		// 32 registers 0 ~ 31
	std::vector<uint8_t> memory;				// byte memorys, byteCounter indicates the index
	std::unordered_map<std::string, int> constants;		// constants, { name, number }
	std::unordered_map<std::string, int> bLabels;		// branch labels, { name, instr_index }
	std::unordered_map<std::string, int> dLabels;		// data labels, { name, memory_address }
	std::unordered_map<std::string, std::string> D_L_type;		// data labels type, { name, type }
	std::vector<Instruction> instructions;
	VM_Status status = VM_Status::Simulating;
	
};



// define the parser module here
enum class InputValue {
	DATA, TEXT, LABEL, CONST_, DELIM, INTEGER, EndL,
	COMMA, EQL, OPEN, CLOSE, WORD, HALF, BYTE, SPACE, STR_LAYOUT,
	CHAR, LS, MOVE, RRS, RR, DIV, BRANCH, LI, NOT,
	J, NOP, COMMENT, REG, branchLabel, dataLabel, Error, EOT
};

enum class StateType {
	init, data_, text_, declaration, word_layout, half_layout,
	byte_layout, space_layout, str_layout, word_comma_end, byte_comma_end,
	half_comma_end, space_comma_end, ls_, move_, rrs_, rr_,
	divi_, branch_, li_, not_, j_, nop_, HALT, ERR
};

static std::set<std::string> load_store{ "lw", "lh", "lb", "la", "sw", "sh", "sb" }; //<opcode> <register> SEP <memref>
static std::set<std::string> move_x{ "mfhi", "mflo", "mthi", "mtlo" };		// <opcode> <register>
static std::set<std::string> two_reg_source{ "add", "addu", "sub", "subu", "mul", "mulo",
											 "mulou", "rem", "remu", "and", "nor", "or", "xor" };	//<opcode> <register> SEP <register> SEP <source> 
static std::set<std::string> reg_sep_reg{ "mult", "multu", "abs", "neg", "negu", "move" };		// <opcode> <register> SEP <register>
static std::set<std::string> divides{ "div", "divu" };		// 1 or 2 reg,  source
static std::set<std::string> branch{ "beq", "bne", "blt", "ble", "bgt", "bge" };	// <opcode> <register> SEP <source> SEP <label>

// "li", "not", "j", "nop"


static std::unordered_map<std::string, int> regAlias{ {"zero", 0}, {"at", 1}, {"v0", 2}, {"v1", 3}, {"a0", 4}, {"a1", 5}, {"a2", 6}, {"a3", 7},
													 {"t0", 8}, {"t1", 9}, {"t2", 10}, {"t3", 11}, {"t4", 12}, {"t5", 13}, {"t6", 14}, {"t7", 15},
													 {"s0", 16}, {"s1", 17}, {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23},
													 {"t8", 24}, {"t9", 25}, {"k0", 26}, {"k1", 27}, {"gp", 28}, {"sp", 29}, {"fp", 30}, {"ra", 31} };

class Parse {
public:

	bool parse(const TokenList& tokens);
	InputValue classify(TokenList::const_iterator it, TokenList::const_iterator end);
	void update0(StateType& state, InputValue input, const TokenList& tokens);
	void update1(StateType& state, InputValue input, const TokenList& tokens);
	void updateWord(StateType& state, InputValue input);
	void updateHalf(StateType& state, InputValue input);
	void updateByte(StateType& state, InputValue input);
	void updateSpace(StateType& state, InputValue input);
	void wordComma(StateType& state, InputValue input, const TokenList& tokens);
	void halfComma(StateType& state, InputValue input, const TokenList& tokens);
	void byteComma(StateType& state, InputValue input, const TokenList& tokens);
	void spaceComma(StateType& state, InputValue input, const TokenList& tokens);
	void update4(StateType& state, InputValue input, const TokenList& tokens);
	void update5(StateType& state, InputValue input, const TokenList& tokens);
	void lsFunc(StateType& state, InputValue input, const TokenList& tokens);
	void moveFunc(StateType& state, InputValue input, const TokenList& tokens);
	void rrsFunc(StateType& state, InputValue input, const TokenList& tokens);
	void rrFunc(StateType& state, InputValue input, const TokenList& tokens);
	void divFunc(StateType& state, InputValue input, const TokenList& tokens);
	void branchFunc(StateType& state, InputValue input, const TokenList& tokens);
	void liFunc(StateType& state, InputValue input, const TokenList& tokens);
	void notFunc(StateType& state, InputValue input, const TokenList& tokens);
	void jFunc(StateType& state, InputValue input);
	void nopFunc(StateType& state, InputValue input);
	int getLine();	// get the current line number
	bool futureLabels(TokenList::const_iterator it, TokenList::const_iterator end, const std::string& str);
	VirtualMachine getVM();
	std::size_t getMainLine();

private:
	InputValue input = InputValue::EOT;
	TokenList::const_iterator it;
	std::size_t lineNum = 0;
	bool data_text = false;		// false for data, true for text section
	bool delimiter = false;
	bool signedV = false;
	bool strNull = false;
	VirtualMachine VM;
	std::string opcodes = "";
	int pins = 0;		// for letting programs to read previous input
	long long int values = 0;
	std::string con = "";
	std::string labelName = "";
	int mainStartLine = 0;
};

bool isNum(const std::string& str);
bool isAlias(const std::string& str);
bool isRegister(const std::string& str);
bool isIntLayout(const std::string& str);
bool isStrLayout(const std::string& str);
bool isChar(const std::string& str);
bool checkConst(const std::string& str);
bool checkLabel(const std::string& str);

// check operation
bool loadStore(const std::string& str);
bool movexx(const std::string& str);
bool reg_reg_source(const std::string& str);
bool reg_reg(const std::string& str);
bool isDiv(const std::string& str);
bool isBranch(const std::string& str);

StateType ops(const InputValue ins);
bool isOP(const InputValue ins);

static std::string uint32ToHex(uint32_t i)
{
	std::stringstream stream;
	stream << "0x"
		<< std::setfill('0') << std::setw(sizeof(i) * 2)
		<< std::hex << i;
	return stream.str();
}
static std::string uint8ToHex(const uint8_t* v) {
	std::stringstream ss;

	ss << std::hex << std::setfill('0');

	for (int i = 0; i < 1; i++) {
		ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
	}

	return "0x" + ss.str();
}




#endif
