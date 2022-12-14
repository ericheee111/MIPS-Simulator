#include "parser.hpp"
#include <iostream>

// implement the parser here
InputValue Parse::classify(TokenList::const_iterator it, TokenList::const_iterator end) {
	InputValue input = InputValue::EOT;

	if (it == end) {
		input = InputValue::EOT;
		return input;
	}

	// punctuatinos
	else if (it->type() == STRING_DELIM)
		input = InputValue::DELIM;
	else if (it->type() == EOL)
		input = InputValue::EndL;
	else if (it->type() == SEP)
		input = InputValue::COMMA;
	else if (it->type() == EQUAL)
		input = InputValue::EQL;
	else if (it->type() == OPEN_PAREN)
		input = InputValue::OPEN;
	else if (it->type() == CLOSE_PAREN)
		input = InputValue::CLOSE;
	else if (it->type() == ERROR)
		input = InputValue::Error;

	// strings
	else if (it->type() == STRING && it->value() == ".data") {
		input = InputValue::DATA;
	}
	else if (it->type() == STRING && it->value() == ".text") {
		input = InputValue::TEXT;
	}
	else if (it->type() == STRING && it->value().back() == ':') {
		if (isalpha(it->value()[0]) && checkLabel(it->value())) {
			input = InputValue::LABEL;
			auto temp = it->value().substr(0, it->value().size() - 1);
			if (data_text == true)
				VM.addBLabels(temp, VM.getInstrIndex());
			else if (data_text == false) {
				VM.addDLabels(temp);
				labelName = temp;
			}

		}
		else
			input = InputValue::Error;
	}
	// check for opcodes below
	else if (it->type() == STRING && loadStore(it->value())) {
		opcodes = it->value();
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::LS;
	}
	else if (it->type() == STRING && movexx(it->value())) {
		opcodes = it->value();
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::MOVE;
	}
	else if (it->type() == STRING && reg_reg_source(it->value())) {
		opcodes = it->value();
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::RRS;
	}
	else if (it->type() == STRING && reg_reg(it->value())) {
		opcodes = it->value();
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::RR;
	}
	else if (it->type() == STRING && isDiv(it->value())) {
		opcodes = it->value();
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::DIV;
	}
	else if (it->type() == STRING && isBranch(it->value())) {
		opcodes = it->value();
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::BRANCH;
	}
	else if (it->type() == STRING && it->value() == "li") {
		opcodes = "li";
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::LI;
	}
	else if (it->type() == STRING && it->value() == "not") {
		opcodes = "not";
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::NOT;
	}
	else if (it->type() == STRING && it->value() == "j") {
		opcodes = "j";
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::J;
	}
	else if (it->type() == STRING && it->value() == "nop") {
		opcodes = "nop";
		if (VM.readInsIndex() == 0) {
			mainStartLine = it->line();
		}
		VM.incre_ins_index();
		input = InputValue::NOP;
	}

	// finished determine operations.

	else if (it->type() == STRING && isRegister(it->value())) {
		auto s = it->value().substr(1, it->value().size() - 1);
		if (isNum(s) == true) {
			pins = stoi(s);
		}
		else if (isAlias(s) == true) {
			pins = regAlias[s];
		}
		input = InputValue::REG;
	}

	// ***************************************************************
	else if (data_text == true && it->type() == STRING && (VM.B_labelExist(it->value()) || futureLabels(it, end, it->value()))) {
		input = InputValue::branchLabel;
		labelName = it->value();
	}
	else if (it->type() == STRING && (VM.D_labelExist(it->value()))) {
		input = InputValue::dataLabel;
		labelName = it->value();
	}

	//xxxxxxxxxxxxxxxxxxxxxxxxxxx

	else if (it->type() == STRING && VM.constExist(it->value())) {
		//values = consts.at(it->value());
		values = VM.readConst(it->value());
		input = InputValue::INTEGER;
	}
	else if (data_text == false && delimiter == false && it->type() == STRING && isalpha(it->value()[0]) && it->value().back() != ':') {
		if (checkConst(it->value())) {
			input = InputValue::CONST_;		// probably a constant in data declaration
			con = it->value();
		}
		else
			input = InputValue::Error;
	}
	else if (it->type() == STRING && (it->value()[0] == '-' || it->value()[0] == '+' || isdigit(it->value()[0]))) {
		std::string temp = it->value().substr(1, it->value().size() - 1);
		bool digit = true;
		for (auto i : temp) {			// check all digit
			if (std::isdigit(i) == 0)
				digit = false;
		}
		if (it->value()[0] == '-' || it->value()[0] == '+')
			signedV = true;
		else if (it->value()[0] != '-' && it->value()[0] != '+')
			signedV = false;
		if (digit == true) {
			values = stoll(it->value());
			input = InputValue::INTEGER;
		}

		else
			input = InputValue::Error;
	}

	else if (it->type() == STRING && isIntLayout(it->value())) {
		if (VM.D_labelExist(labelName) == false)
			VM.addDLType(labelName, it->value());		// store the type of label indicates how many bytes used
		if (it->value() == ".word") {
			input = InputValue::WORD;
		}
		else if (it->value() == ".half") {
			input = InputValue::HALF;
		}
		else if (it->value() == ".byte") {
			input = InputValue::BYTE;
		}
		else if (it->value() == ".space") {
			input = InputValue::SPACE;
		}
	}
	else if (it->type() == STRING && isStrLayout(it->value())) {
		if (VM.D_labelExist(labelName) == false)
			VM.addDLType(labelName, it->value());		// store the type of label indicates how many bytes used
		if (it->value().back() == 'z') {
			strNull = true;
		}
		else
			strNull = false;
		input = InputValue::STR_LAYOUT;
	}
	else if (delimiter == true && it->type() == STRING && isChar(it->value())) {
		input = InputValue::CHAR;
	}
	else
		input = InputValue::Error;

	lineNum = it->line();
	return input;
}

bool Parse::futureLabels(TokenList::const_iterator it, TokenList::const_iterator end, const std::string& str) {
	auto temp = it;
	auto temp_instri = VM.getInstrIndex();
	bool found = false;
	do {
		std::string t = temp->value().substr(0, temp->value().size() - 1);
		if (temp->type() == STRING && temp->value().back() == ':' && t == str) {
			VM.addBLabels(t, temp_instri);
			found = true;
			break;
		}
		else if (temp->type() == STRING && (loadStore(temp->value()) || movexx(temp->value()) || reg_reg_source(temp->value()) || reg_reg(temp->value()) || isDiv(temp->value()) || isBranch(temp->value()) || temp->value() == "li" || temp->value() == "not" || temp->value() == "j" || temp->value() == "nop")) {
			temp_instri++;
			temp++;
		}
		else
			temp++;

	} while (temp != end);

	return found;
}


bool Parse::parse(const TokenList& tokens) {
	StateType state = StateType::init;
	it = tokens.begin();

	do {

		input = classify(it, tokens.end());

		if (input == InputValue::EOT)
			break;
		else if (input == InputValue::Error)
			state = StateType::ERR;
		it++;

		switch (state) {
		case StateType::init:
			update0(state, input, tokens);
			break;
		case StateType::data_:
			update1(state, input, tokens);
			break;
		case StateType::word_layout:
			updateWord(state, input);
			break;
		case StateType::half_layout:
			updateHalf(state, input);
			break;
		case StateType::byte_layout:
			updateByte(state, input);
			break;
		case StateType::space_layout:
			updateSpace(state, input);
			break;
		case StateType::word_comma_end:
			wordComma(state, input, tokens);
			break;
		case StateType::half_comma_end:
			halfComma(state, input, tokens);
			break;
		case StateType::byte_comma_end:
			byteComma(state, input, tokens);
			break;
		case StateType::space_comma_end:
			spaceComma(state, input, tokens);
			break;
		case StateType::str_layout:
			update4(state, input, tokens);
			break;
		case StateType::text_:
			update5(state, input, tokens);
			break;
		case StateType::ls_:
			lsFunc(state, input, tokens);
			break;
		case StateType::move_:
			moveFunc(state, input, tokens);
			break;
		case StateType::rrs_:
			rrsFunc(state, input, tokens);
			break;
		case StateType::rr_:
			rrFunc(state, input, tokens);
			break;
		case StateType::divi_:
			divFunc(state, input, tokens);
			break;
		case StateType::branch_:
			branchFunc(state, input, tokens);
			break;
		case StateType::li_:
			liFunc(state, input, tokens);
			break;
		case StateType::not_:
			notFunc(state, input, tokens);
			break;
		case StateType::j_:
			jFunc(state, input);
			break;
		case StateType::nop_:
			nopFunc(state, input);
			break;
		case StateType::HALT:
			break;

		default: state = StateType::ERR;
		}


	} while ((state != StateType::ERR && state != StateType::HALT));

	VM.fillMEM();

	return state != StateType::ERR;
}

void Parse::update0(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::init);

	switch (input) {
	case InputValue::DATA:
		data_text = false;
		input = classify(it, tokens.end());
		if (input == InputValue::EOT)
			break;
		it++;

		if (input == InputValue::EndL)		// check next token
			state = StateType::data_;		// data section
		else
			state = StateType::ERR;
		break;
	case InputValue::TEXT:
		input = classify(it, tokens.end());
		if (input == InputValue::EOT)
			break;
		it++;

		if (input == InputValue::EndL)		// check next token
			state = StateType::text_;		// text section
		else
			state = StateType::ERR;
		break;
	case InputValue::EOT:
		state = StateType::HALT;
		break;
	case InputValue::EndL:
		state = StateType::init;
		break;
	default: state = StateType::ERR;
	}
}

void Parse::update1(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::data_);
	data_text = false;
	switch (input) {
	case InputValue::CONST_:	// delaring constant
		input = classify(it, tokens.end());
		if (input == InputValue::EOT)
			break;
		it++;

		if (input == InputValue::EQL) {		// check next token is EQUAL
			input = classify(it, tokens.end());
			if (input == InputValue::INTEGER) {
				//consts.insert({ con, stoll(it->value()) });
				VM.addConsts(con, stoll(it->value()));
			}
			if (input == InputValue::EOT)
				break;
			it++;

			if (input == InputValue::INTEGER) {		// check next token is integer
				input = classify(it, tokens.end());
				if (input == InputValue::EOT)
					break;
				it++;

				if (input == InputValue::EndL)
					state = StateType::data_;
				else
					state = StateType::ERR;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
		break;
	case InputValue::LABEL:		// declaring label
		input = classify(it, tokens.end());
		if (input == InputValue::EOT)
			break;
		it++;

		if (input == InputValue::WORD) {		// following an int layout
			state = StateType::word_layout;
		}
		else if (input == InputValue::HALF) {
			state = StateType::half_layout;
		}
		else if (input == InputValue::BYTE) {
			state = StateType::byte_layout;
		}
		else if (input == InputValue::SPACE) {
			state = StateType::space_layout;
		}
		else if (input == InputValue::STR_LAYOUT) {	// following a string layout
			state = StateType::str_layout;
		}
		else if (input == InputValue::EndL) {		// following an EOL
			state = StateType::data_;
		}
		else
			state = StateType::ERR;



		break;
	case InputValue::WORD:	// declaring directly int layout .word
		state = StateType::word_layout;
		break;
	case InputValue::HALF:	// declaring directly int layout .half
		state = StateType::half_layout;
		break;
	case InputValue::BYTE:	// declaring directly int layout .byte
		state = StateType::byte_layout;
		break;
	case InputValue::SPACE:	// declaring directly int layout .space
		state = StateType::space_layout;
		break;

	case InputValue::STR_LAYOUT:	// declaring directly string layout
		state = StateType::str_layout;
		break;
	case InputValue::TEXT:
		data_text = true;
		input = classify(it, tokens.end());
		if (input == InputValue::EOT)
			break;
		it++;

		if (input == InputValue::EndL)		// check next token
			state = StateType::text_;		// text section
		else
			state = StateType::ERR;
		break;
	case InputValue::EndL:
		state = StateType::data_;
		break;
	default: state = StateType::ERR;
	}
}


void Parse::updateWord(StateType& state, InputValue input) {
	assert(state == StateType::word_layout);

	if (input == InputValue::INTEGER) {
		if (signedV == true && values >= (long int)-2147483648 && values <= (long int)2147483647) {
			state = StateType::word_comma_end;
			VM.pushMem(values, 4);
		}
		else if (signedV == false && values >= 0 && values <= 4294967295) {
			state = StateType::word_comma_end;
			VM.pushMem(values, 4);
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::updateHalf(StateType& state, InputValue input) {
	assert(state == StateType::half_layout);

	if (input == InputValue::INTEGER && signedV == true && values >= -32768 && values <= 32767) {
		state = StateType::half_comma_end;
		VM.pushMem(values, 2);
	}
	else if (input == InputValue::INTEGER && signedV == false && values >= 0 && values <= 65535) {
		state = StateType::half_comma_end;
		VM.pushMem(values, 2);
	}
	else
		state = StateType::ERR;
}

void Parse::updateByte(StateType& state, InputValue input) {
	assert(state == StateType::byte_layout);

	if (input == InputValue::INTEGER && signedV == true && values >= -128 && values <= 127) {
		state = StateType::byte_comma_end;
		VM.pushMem(values, 1);
	}
	else if (input == InputValue::INTEGER && signedV == false && values >= 0 && values <= 255) {
		state = StateType::byte_comma_end;
		VM.pushMem(values, 1);
	}
	else
		state = StateType::ERR;
}

void Parse::updateSpace(StateType& state, InputValue input) {
	assert(state == StateType::space_layout);

	if (input == InputValue::INTEGER && signedV == true && values >= (long int)-2147483648 && values <= (long int)2147483647) {
		state = StateType::space_comma_end;
		VM.pushMemSpace(values);
	}
	else if (input == InputValue::INTEGER && signedV == false && values >= 0 && values <= 4294967295) {
		state = StateType::space_comma_end;
		VM.pushMemSpace(values);
	}
	else
		state = StateType::ERR;
}

void Parse::wordComma(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::word_comma_end);
	if (input == InputValue::COMMA) {
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::INTEGER) {
			if (signedV == true && values >= (long int)-2147483648 && values <= (long int)2147483647) {
				state = StateType::word_comma_end;
				VM.pushMem(values, 4);
			}
			else if (signedV == false && values >= 0 && values <= 4294967295) {
				state = StateType::word_comma_end;
				VM.pushMem(values, 4);
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;

	}
	else if (input == InputValue::EndL)
		state = StateType::data_;
	else
		state = StateType::ERR;
}

void Parse::halfComma(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::half_comma_end);
	if (input == InputValue::COMMA) {
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::INTEGER && signedV == true && values >= -32768 && values <= 32767) {
			state = StateType::half_comma_end;
			VM.pushMem(values, 2);
		}
		else if (input == InputValue::INTEGER && signedV == false && values >= 0 && values <= 65535) {
			state = StateType::half_comma_end;
			VM.pushMem(values, 2);
		}
		else
			state = StateType::ERR;

	}
	else if (input == InputValue::EndL)
		state = StateType::data_;
	else
		state = StateType::ERR;
}

void Parse::byteComma(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::byte_comma_end);
	if (input == InputValue::COMMA) {
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::INTEGER && signedV == true && values >= -128 && values <= 127) {
			state = StateType::byte_comma_end;
			VM.pushMem(values, 1);
		}
		else if (input == InputValue::INTEGER && signedV == false && values >= 0 && values <= 255) {
			state = StateType::byte_comma_end;
			VM.pushMem(values, 1);
		}
		else
			state = StateType::ERR;

	}
	else if (input == InputValue::EndL)
		state = StateType::data_;
	else
		state = StateType::ERR;
}

void Parse::spaceComma(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::space_comma_end);
	if (input == InputValue::COMMA) {
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::INTEGER && signedV == true && values >= (long int)-2147483648 && values <= (long int)2147483647) {
			state = StateType::space_comma_end;
			VM.pushMemSpace(values);
		}
		else if (input == InputValue::INTEGER && signedV == false && values >= 0 && values <= 4294967295) {
			state = StateType::space_comma_end;
			VM.pushMemSpace(values);
		}
		else
			state = StateType::ERR;

	}
	else if (input == InputValue::EndL)
		state = StateType::data_;
	else
		state = StateType::ERR;
}


void Parse::update4(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::str_layout);

	if (input == InputValue::DELIM) {
		delimiter = true;
		input = classify(it, tokens.end());
		if (input == InputValue::CHAR) {
			VM.pushStrMem(it->value());
			if (strNull == true) {
				VM.pushStrMem("0");
				strNull = false;
			}

		}
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::CHAR) {
			input = classify(it, tokens.end());
			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::DELIM) {
				delimiter = false;
				input = classify(it, tokens.end());
				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::EndL)
					state = StateType::data_;
				else if (input == InputValue::EOT)
					state = StateType::HALT;
				else
					state = StateType::ERR;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::update5(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::text_);

	if (input == InputValue::LABEL) {
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::EndL) {
			state = StateType::text_;
		}
		else if (isOP(input))
			state = ops(input);
		else
			state = StateType::ERR;
	}
	else if (isOP(input)) {
		state = ops(input);
	}
	else if (input == InputValue::EndL)
		state = StateType::text_;
	else
		state = StateType::ERR;

}

//**************
void Parse::lsFunc(StateType& state, InputValue input, const TokenList& tokens) {		// load and store instructions
	assert(state == StateType::ls_);

	if (input == InputValue::REG) {
		Instruction ls;
		ls.setOp(opcodes);
		ls.setRT(pins);
		ls.setLine(lineNum);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::COMMA) {
			input = classify(it, tokens.end());

			//xxxxxxxxxxxxxxx   add values to members of instruction xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx down
			if (input == InputValue::dataLabel) {

				auto address = VM.readDLabel(it->value());
				auto type = VM.readDLType(it->value());
				auto size = 0;
				if (type == ".word" || type == ".space")
					size = 4;
				else if (type == ".half")
					size = 2;
				else if (type == ".byte")
					size = 1;

				//uint32_t mem = VM.readMEM(address, size);
				ls.setmemsize(size);
				ls.setMemref(address);
			}
			else if (input == InputValue::REG) {

				ls.setRS(pins);
			}
			else if (input == InputValue::INTEGER) {

				auto imme = it->value();
				if (isNum(imme) == false) {
					auto data = VM.readConst(imme);
					ls.setImm(data);
				}
				else
					ls.setImm(stoi(imme));
			}
			//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx up

			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::OPEN) {
				ls.setOffset(true);
				input = classify(it, tokens.end());

				// xxxxxxxxxxxxxxxxxxxxxx add values to members of instruction  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx down 
				if (input == InputValue::dataLabel) {
					ls.setMemref(VM.readDLabel(it->value()));
				}
				else if (input == InputValue::REG) {
					ls.setRS(pins);
				}
				else if (input == InputValue::INTEGER) {
					auto imme = it->value();
					if (isNum(imme) == false) {
						auto data = VM.readConst(imme);
						ls.setMemref(data);
					}
					else
						ls.setMemref(stoi(imme));
				}
				// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx up

				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::REG || input == InputValue::dataLabel || input == InputValue::INTEGER) {
					input = classify(it, tokens.end());
					if (input != InputValue::EOT)
						it++;
					if (input == InputValue::CLOSE) {
						VM.pushInstruction(ls);				// push to sequence
						state = StateType::text_;
					}
					else
						state = StateType::ERR;
				}
				else
					state = StateType::ERR;
			}
			else if (input == InputValue::dataLabel || input == InputValue::REG) {
				input = classify(it, tokens.end());

				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::EndL) {
					VM.pushInstruction(ls);				// push to sequence

					state = StateType::text_;
				}
				else
					state = StateType::ERR;
			}
			else if (input == InputValue::INTEGER) {			// integer is also an offset
				input = classify(it, tokens.end());
				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::OPEN) {
					input = classify(it, tokens.end());

					ls.setOffset(true);
					// xxxxxxxxxxxxxxxxxxxxxx add values to members of instruction  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx down
					if (input == InputValue::dataLabel) {
						ls.setMemref(VM.readDLabel(it->value()));		// byte address
					}
					else if (input == InputValue::REG) {
						ls.setRS(pins);							// reg stores byte address
					}
					else if (input == InputValue::INTEGER) {
						auto imme = it->value();
						if (isNum(imme) == false) {
							auto data = VM.readConst(imme);
							ls.setMemref(data);					// byte address
						}
						else
							ls.setMemref(stoi(imme));
					}
					// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx up


					if (input != InputValue::EOT)
						it++;
					if (input == InputValue::REG || input == InputValue::dataLabel || input == InputValue::INTEGER) {
						input = classify(it, tokens.end());
						if (input != InputValue::EOT)
							it++;
						if (input == InputValue::CLOSE) {
							VM.pushInstruction(ls);				// push to sequence
							state = StateType::text_;
						}
						else
							state = StateType::ERR;
					}
					else
						state = StateType::ERR;
				}
				else if (input == InputValue::EndL) {
					VM.pushInstruction(ls);				// push to sequence
					state = StateType::text_;
				}
				else
					state = StateType::ERR;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::moveFunc(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::move_);

	if (input == InputValue::REG) {
		Instruction move;
		move.setOp(opcodes);
		move.setRD(pins);
		move.setLine(lineNum);
		VM.pushInstruction(move);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::EndL) {
			state = StateType::text_;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::rrsFunc(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::rrs_);

	if (input == InputValue::REG) {
		Instruction rrs;
		rrs.setOp(opcodes);
		rrs.setRD(pins);
		rrs.setLine(lineNum);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::COMMA) {
			input = classify(it, tokens.end());
			if (input == InputValue::REG) {
				rrs.setRS(pins);
			}
			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::REG) {
				input = classify(it, tokens.end());
				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::COMMA) {
					input = classify(it, tokens.end());
					if (input == InputValue::REG) {
						rrs.setRT(pins);
					}
					else if (input == InputValue::INTEGER) {
						auto imme = it->value();
						if (isNum(imme) == false) {
							auto data = VM.readConst(imme);
							rrs.setImm(data);
						}
						else
							rrs.setImm(stoi(imme));
					}

					if (input != InputValue::EOT)
						it++;
					if (input == InputValue::REG || input == InputValue::INTEGER) {
						VM.pushInstruction(rrs);
						state = StateType::text_;
					}
					else
						state = StateType::ERR;
				}
				else
					state = StateType::ERR;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::rrFunc(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::rr_);

	if (input == InputValue::REG) {
		Instruction rr;
		rr.setOp(opcodes);
		rr.setRD(pins);
		rr.setLine(lineNum);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::COMMA) {
			input = classify(it, tokens.end());
			if (input == InputValue::REG) {
				rr.setRS(pins);
			}
			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::REG) {
				VM.pushInstruction(rr);
				state = StateType::text_;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;

}

void Parse::divFunc(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::divi_);

	if (input == InputValue::REG) {
		Instruction div;
		div.setOp(opcodes);
		div.setRD(pins);
		div.setLine(lineNum);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::COMMA) {
			input = classify(it, tokens.end());

			if (input == InputValue::REG) {
				div.setRS(pins);
			}

			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::REG) {
				input = classify(it, tokens.end());
				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::COMMA) {
					input = classify(it, tokens.end());

					if (input == InputValue::REG) {
						div.setRS(pins);
					}
					else if (input == InputValue::INTEGER) {
						auto imme = it->value();
						if (isNum(imme) == false) {
							auto data = VM.readConst(imme);
							div.setImm(data);
						}
						else
							div.setImm(stoi(imme));
					}

					if (input != InputValue::EOT)
						it++;
					if (input == InputValue::REG || input == InputValue::INTEGER) {
						VM.pushInstruction(div);

						state = StateType::text_;
					}
					else
						state = StateType::ERR;
				}
				else if (input == InputValue::EndL) {
					VM.pushInstruction(div);

					state = StateType::text_;
				}
				else
					state = StateType::ERR;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::branchFunc(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::branch_);

	if (input == InputValue::REG) {
		Instruction branch;
		branch.setOp(opcodes);
		branch.setRS(pins);
		branch.setLine(lineNum);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::COMMA) {
			input = classify(it, tokens.end());

			if (input == InputValue::REG) {
				branch.setRT(pins);
			}
			else if (input == InputValue::INTEGER) {
				auto imme = it->value();
				if (isNum(imme) == false) {
					auto data = VM.readConst(imme);
					branch.setImm(data);
				}
				else
					branch.setImm(stoi(imme));
			}

			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::REG || input == InputValue::INTEGER) {
				input = classify(it, tokens.end());
				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::COMMA) {
					input = classify(it, tokens.end());

					if (input == InputValue::branchLabel) {
						branch.setMemref(VM.readBLabel(it->value()));
					}

					if (input != InputValue::EOT)
						it++;
					if (input == InputValue::branchLabel) {
						VM.pushInstruction(branch);

						state = StateType::text_;
					}
					else
						state = StateType::ERR;
				}
				else
					state = StateType::ERR;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;

}

void Parse::liFunc(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::li_);

	if (input == InputValue::REG) {
		Instruction li;
		li.setOp(opcodes);
		li.setRT(pins);
		li.setLine(lineNum);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::COMMA) {
			input = classify(it, tokens.end());
			if (input == InputValue::INTEGER) {
				auto imme = it->value();
				if (isNum(imme) == false) {
					auto data = VM.readConst(imme);
					li.setImm(data);
				}
				else
					li.setImm(stoi(imme));
			}

			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::INTEGER) {
				input = classify(it, tokens.end());
				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::EndL) {
					VM.pushInstruction(li);
					state = StateType::text_;
				}
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::notFunc(StateType& state, InputValue input, const TokenList& tokens) {
	assert(state == StateType::not_);

	if (input == InputValue::REG) {
		Instruction _not;
		_not.setOp(opcodes);
		_not.setRD(pins);
		_not.setLine(lineNum);
		input = classify(it, tokens.end());
		if (input != InputValue::EOT)
			it++;
		if (input == InputValue::COMMA) {
			input = classify(it, tokens.end());

			if (input == InputValue::REG) {
				_not.setRS(pins);
			}
			else if (input == InputValue::INTEGER) {
				auto imme = it->value();
				if (isNum(imme) == false) {
					auto data = VM.readConst(imme);
					_not.setImm(data);
				}
				else
					_not.setImm(stoi(imme));
			}

			if (input != InputValue::EOT)
				it++;
			if (input == InputValue::REG || input == InputValue::INTEGER) {
				input = classify(it, tokens.end());
				if (input != InputValue::EOT)
					it++;
				if (input == InputValue::EndL) {
					VM.pushInstruction(_not);
					state = StateType::text_;
				}
				else
					state = StateType::ERR;
			}
			else
				state = StateType::ERR;
		}
		else
			state = StateType::ERR;
	}
	else
		state = StateType::ERR;
}

void Parse::jFunc(StateType& state, InputValue input) {
	assert(state == StateType::j_);

	if (input == InputValue::branchLabel) {
		Instruction jump;
		jump.setOp(opcodes);
		jump.setMemref(VM.readBLabel(labelName));
		jump.setLine(lineNum);
		VM.pushInstruction(jump);
		state = StateType::text_;
	}
	else
		state = StateType::ERR;

}

void Parse::nopFunc(StateType& state, InputValue input) {
	assert(state == StateType::nop_);

	if (input == InputValue::EndL) {
		Instruction nop;
		nop.setOp(opcodes);
		nop.setLine(lineNum);
		VM.pushInstruction(nop);
		state = StateType::text_;
	}
	else
		state = StateType::ERR;

}

// define input
bool isNum(const std::string& str) {
	for (auto i : str) {
		if (isdigit(i) == 0)
			return false;
	}
	return true;
}

bool isAlias(const std::string& str) {
	if (regAlias.find(str) != regAlias.end()) {
		return true;
	}
	else
		return false;
}


bool isRegister(const std::string& str) {
	if (str[0] == '$') {
		auto s = str.substr(1, str.size() - 1);
		if (isNum(s) == true) {
			int reg = stoi(s);
			if (reg >= 0 && reg <= 31) {
				return true;
			}
			else
				return false;
		}
		else if (isAlias(s) == true) {
			return true;
		}
		else
			return false;
	}
	else
		return false;
}


bool isIntLayout(const std::string& str) {
	return (str == ".word" || str == ".half" || str == ".byte" || str == ".space");
}
bool isStrLayout(const std::string& str) {
	return (str == ".ascii" || str == ".asciiz");
}
bool isChar(const std::string& str) {
	for (std::size_t i = 0; i < str.size(); i++) {
		if (isprint(str[i]) == 0)
			return false;
	}
	return true;
}
bool checkConst(const std::string& str) {
	for (auto i : str) {
		if (isdigit(i) == 0 && isalpha(i) == 0)
			return false;
	}
	return true;
}
bool checkLabel(const std::string& str) {
	for (std::size_t i = 0; i < str.size() - 1; i++) {
		if (isdigit(str[i]) == 0 && isalpha(str[i]) == 0)
			return false;
	}
	return true;
}

bool loadStore(const std::string& str) {
	if (load_store.find(str) != load_store.end()) {
		return true;
	}
	else
		return false;
}

bool movexx(const std::string& str) {
	if (move_x.find(str) != move_x.end()) {
		return true;
	}
	else
		return false;
}

bool reg_reg_source(const std::string& str) {
	if (two_reg_source.find(str) != two_reg_source.end()) {
		return true;
	}
	else
		return false;
}

bool reg_reg(const std::string& str) {
	if (reg_sep_reg.find(str) != reg_sep_reg.end()) {
		return true;
	}
	else
		return false;
}

bool isDiv(const std::string& str) {
	if (divides.find(str) != divides.end()) {
		return true;
	}
	else
		return false;
}

bool isBranch(const std::string& str) {
	if (branch.find(str) != branch.end()) {
		return true;
	}
	else
		return false;
}

bool isOP(const InputValue ins) {
	if (ins == InputValue::LS || ins == InputValue::MOVE || ins == InputValue::RRS || ins == InputValue::RR || ins == InputValue::DIV || ins == InputValue::BRANCH || ins == InputValue::LI || ins == InputValue::NOT || ins == InputValue::J || ins == InputValue::NOP) {
		return true;
	}
	else
		return false;
}

// define operation and assign to state
StateType ops(const InputValue ins) {
	switch (ins) {
	case InputValue::LS:
		return StateType::ls_;
		break;
	case InputValue::MOVE:
		return StateType::move_;
		break;
	case InputValue::RRS:
		return StateType::rrs_;
		break;
	case InputValue::RR:
		return StateType::rr_;
		break;
	case InputValue::DIV:
		return StateType::divi_;
		break;
	case InputValue::BRANCH:
		return StateType::branch_;
		break;
	case InputValue::LI:
		return StateType::li_;
		break;
	case InputValue::NOT:
		return StateType::not_;
		break;
	case InputValue::J:
		return StateType::j_;
		break;
	case InputValue::NOP:
		return StateType::nop_;
		break;
	default: return StateType::ERR;
	}
}

int Parse::getLine() {
	return lineNum;
}

void VirtualMachine::addBLabels(std::string str, long int i) {
	bLabels.insert({ str, i });
}
void VirtualMachine::addDLabels(std::string str) {
	dLabels.insert({ str, byteCounter });
}
void VirtualMachine::addDLType(std::string str, std::string type) {
	D_L_type.insert({ str, type });
}

void VirtualMachine::addConsts(std::string str, long int i) {
	constants.insert({ str, i });
}
bool VirtualMachine::D_labelExist(std::string str) {
	return (dLabels.find(str) != dLabels.end());
}
bool VirtualMachine::B_labelExist(std::string str) {
	return (bLabels.find(str) != bLabels.end());
}
bool VirtualMachine::constExist(std::string str) {
	return (constants.find(str) != constants.end());
}
long int VirtualMachine::readConst(std::string str) {
	return constants[str];
}
long int VirtualMachine::readDLabel(std::string str) {
	return dLabels[str];
}
std::string VirtualMachine::readDLType(std::string str) {
	return D_L_type[str];
}

long int VirtualMachine::readBLabel(std::string str) {

	return bLabels[str];
}

void VirtualMachine::pushMem(long int values, int size) {
	uint8_t temp = 0;
	while (size > 0) {
		size--;
		temp = values & 0xFF;
		memory.push_back(temp);
		values >>= 8;
		byteCounter++;
	}
}
void VirtualMachine::pushMemSpace(int size) {
	while (size > 0) {
		size--;
		memory.push_back((uint8_t)0);
		byteCounter++;
	}
}

void VirtualMachine::pushStrMem(std::string str) {
	int length = str.size();

	for (int i = 0; i < length - 1; i++) {
		memory.push_back(str[i]);
		byteCounter++;
	}
}

void VirtualMachine::pushInstruction(Instruction instr) {
	instructions.push_back(instr);
}

uint16_t VirtualMachine::getInstrIndex() {
	return instru_index;
}
void VirtualMachine::incre_ins_index() {
	instru_index++;
}
int VirtualMachine::readInsIndex() {
	return instru_index;
}

void Instruction::setOp(std::string str) {
	opcode = str;
}
void Instruction::setRS(int irs) {
	urs = true;
	rs = findReg(irs);
}
void Instruction::setRT(int irt) {
	urt = true;
	rt = findReg(irt);
}
void Instruction::setRD(int ird) {
	urd = true;
	rd = findReg(ird);
}
void Instruction::setImm(long int x) {
	uimm = true;
	immediate = x;
}
void Instruction::setOffset(bool x) {
	offset = x;
}
void Instruction::setMemref(int x) {
	umemref = true;
	memref = x;
}
void Instruction::setmemsize(int x) {
	memsize = x;
}
void Instruction::setLine(std::size_t lineNum) {
	line_number = lineNum;
}

std::string Instruction::readOP() {
	return opcode;
}

Register Instruction::readRS() {
	return rs;
}
Register Instruction::readRT() {
	return rt;
}
Register Instruction::readRD() {
	return rd;
}
uint32_t Instruction::readIMM() {
	return immediate;
}
uint32_t Instruction::readMEMREF() {
	return memref;
}

int Instruction::readLineNum() {
	return line_number;
}

int Instruction::getMemSize() {
	return memsize;
}
bool Instruction::readOffset() {
	return offset;
}

uint32_t VirtualMachine::readMEM(uint16_t address, int size) {
	uint32_t sol = 0;
	address = address + size - 1;
	while (size > 0) {
		size--;
		auto temp = memory[address];
		sol = sol << 8;
		sol = sol | temp;
		address--;
	}

	return sol;
}
void VirtualMachine::fillMEM() {
	std::vector<uint8_t> fill(512);
	memory.insert(std::end(memory), std::begin(fill), std::end(fill));
}

uint32_t VirtualMachine::readReg(int i) {
	return reg[i];
}
uint32_t VirtualMachine::readPC() {
	return pc;
}
uint32_t VirtualMachine::readHI() {
	return hi;
}
uint32_t VirtualMachine::readLO() {
	return lo;
}
VM_Status VirtualMachine::getStatus() {
	return status;
}

Instruction VirtualMachine::getInstruction(int index) {
	return instructions[index];
}

std::vector<Instruction> VirtualMachine::getInstrVector() {
	return instructions;
}

bool Instruction::readUMem() {
	return umemref;
}

bool Instruction::readURS() {
	return urs;
}
bool Instruction::readURT() {
	return urt;
}
bool Instruction::readURD() {
	return urd;
}

bool Instruction::readUIMM() {
	return uimm;
}

Register Instruction::findReg(int reg) {
	switch (reg) {
	case 0:
		return R0;
		break;
	case 1:
		return R1;
		break;
	case 2:
		return R2;
		break;
	case 3:
		return R3;
		break;
	case 4:
		return R4;
		break;
	case 5:
		return R5;
		break;
	case 6:
		return R6;
		break;
	case 7:
		return R7;
		break;
	case 8:
		return R8;
		break;
	case 9:
		return R9;
		break;
	case 10:
		return R10;
		break;
	case 11:
		return R11;
		break;
	case 12:
		return R12;
		break;
	case 13:
		return R13;
		break;
	case 14:
		return R14;
		break;
	case 15:
		return R15;
		break;
	case 16:
		return R16;
		break;
	case 17:
		return R17;
		break;
	case 18:
		return R18;
		break;
	case 19:
		return R19;
		break;
	case 20:
		return R20;
		break;
	case 21:
		return R21;
		break;
	case 22:
		return R22;
		break;
	case 23:
		return R23;
		break;
	case 24:
		return R24;
		break;
	case 25:
		return R25;
		break;
	case 26:
		return R26;
		break;
	case 27:
		return R27;
		break;
	case 28:
		return R28;
		break;
	case 29:
		return R29;
		break;
	case 30:
		return R30;
		break;
	case 31:
		return R31;
		break;
	default:
		return errors;
	}
}

VirtualMachine Parse::getVM() {
	return VM;
}
std::size_t Parse::getMainLine() {
	return mainStartLine;
}

void VirtualMachine::calculation(Instruction ins, VM_Status status) {
	assert(status == VM_Status::Simulating);
	std::string op = ins.readOP();

	if (op == "nop") {
		pc++;
	}
	else if (op == "lw") {
		auto RT = ins.readRT();			// get register
		auto RTNum = regi[RT];			// find register number
		auto address = 0;
		uint32_t temp = 0;
		auto off = ins.readIMM();
		if (ins.readUMem() == true) {		// label byte address and immediate inside parenthesis are put into MEMREF
			temp = ins.readMEMREF();
			address = temp + off;
		}
		else if (ins.readURS() == true) {
			auto regNum = regi[ins.readRS()];
			temp = reg[regNum];
			address = temp + off;
		}
		else {
			status = VM_Status::Error;
		}

		if (status == VM_Status::Simulating) {
			reg[RTNum] = readMEM(address, 4);
		}
		pc++;
	}
	else if (op == "li") {
		auto RT = ins.readRT();			// find register
		if (RT == errors || RT == empty) {
			status = VM_Status::Error;
		}
		else {
			auto regNum = regi[RT];			// find register number
			auto imm = ins.readIMM();		// get immediate

			reg[regNum] = imm;				// load register
			pc++;
		}
	}
	else if (op == "la") {		// xxxxxxxxxxxxxxxxxxxxxxxxxxx   need verify xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		Register RT = ins.readRT();			// get register
		int RTNum = regi[RT];			// find register number
		if (ins.readOffset() == false) {
			uint32_t temp = 0;
			if (ins.readUMem() == true) {
				temp = ins.readMEMREF();
			}
			else if (ins.readURS() == true) {
				auto regNum = regi[ins.readRS()];
				temp = reg[regNum];
			}
			else if (ins.readUIMM() == true) {
				temp = ins.readIMM();
			}
			else {
				status = VM_Status::Error;
			}

			if (status == VM_Status::Simulating) {
				reg[RTNum] = temp;
			}
			pc++;
		}
		else if (ins.readOffset() == true) {
			auto address = 0;
			uint32_t temp = 0;
			auto off = ins.readIMM();
			if (ins.readUMem() == true) {		// label byte address and immediate inside parenthesis are put into MEMREF
				temp = ins.readMEMREF();
				address = temp + off;
			}
			else if (ins.readURS() == true) {
				auto regNum = regi[ins.readRS()];
				temp = reg[regNum];
				address = temp + off;
			}
			else {
				status = VM_Status::Error;
			}

			if (status == VM_Status::Simulating) {
				reg[RTNum] = address;
			}
			pc++;
		}
		else {
			status = VM_Status::Error;
		}
	}
	else if (op == "sw") {
		auto RT = ins.readRT();			// get register
		auto RTNum = regi[RT];			// find register number
		uint32_t regV = reg[RTNum];		// get the value in register

		if (ins.readOffset() == false) {
			uint32_t address = 0;
			if (ins.readUMem() == true) {
				address = ins.readMEMREF();
			}
			else if (ins.readURS() == true) {
				auto regNum = regi[ins.readRS()];
				address = reg[regNum];
			}
			else if (ins.readUIMM() == true) {
				address = ins.readIMM();
			}
			else {
				status = VM_Status::Error;
			}

			if (status == VM_Status::Simulating && address < (uint32_t)memory.size()) {
				memory[address] = regV & 0xFF;
				regV >>= 8;
				memory[address + 1] = regV & 0xFF;
				regV >>= 8;
				memory[address + 2] = regV & 0xFF;
				regV >>= 8;
				memory[address + 3] = regV & 0xFF;
				pc++;
			}
			else {
				status = VM_Status::Error;
			}

		}
		else if (ins.readOffset() == true) {
			auto address = 0;
			uint32_t temp = 0;
			auto off = ins.readIMM();
			if (ins.readUMem() == true) {		// label byte address and immediate inside parenthesis are put into MEMREF
				temp = ins.readMEMREF();
				address = temp + off;
			}
			else if (ins.readURS() == true) {
				auto regNum = regi[ins.readRS()];
				temp = reg[regNum];
				address = temp + off;
			}
			else {
				status = VM_Status::Error;
			}

			if (status == VM_Status::Simulating && address < (int)memory.size()) {
				memory[address] = regV & 0xFF;
				regV >>= 8;
				memory[address + 1] = regV & 0xFF;
				regV >>= 8;
				memory[address + 2] = regV & 0xFF;
				regV >>= 8;
				memory[address + 3] = regV & 0xFF;
				pc++;
			}
			else {
				status = VM_Status::Error;
			}
		}
		else {
			status = VM_Status::Error;
		}
	}
	else if (op == "move") {
		auto RD = ins.readRD();			// find register RD
		auto RS = ins.readRS();			// find register RS
		if (RD == errors || RD == empty || RS == errors || RS == empty) {
			status = VM_Status::Error;
		}
		else {
			auto RDNum = regi[RD];			// find register number
			auto RSNum = regi[RS];			// find register number

			reg[RDNum] = reg[RSNum];		// load RD with RS
			pc++;
		}


	}
	else if (op == "mfhi") {
		auto RD = ins.readRD();			// find register RD
		if (RD == errors || RD == empty) {
			status = VM_Status::Error;
		}
		else {
			auto RDNum = regi[RD];
			reg[RDNum] = hi;			// load RD with high register
			pc++;
		}
	}
	else if (op == "mflo") {
		auto RD = ins.readRD();			// find register RD
		if (RD == errors || RD == empty) {
			status = VM_Status::Error;
		}
		else {
			auto RDNum = regi[RD];
			reg[RDNum] = lo;			// load RD with low register
			pc++;
		}
	}

	else if (op == "add" || op == "addu" || op == "sub" || op == "subu" || op == "and" || op == "nor" || op == "or" || op == "xor") {
		auto RD = ins.readRD();
		auto RS = ins.readRS();
		uint32_t source = 0;

		if (ins.readURT()) {
			auto RT = ins.readRT();
			auto RTNum = regi[RT];
			source = reg[RTNum];
		}
		else if (ins.readUIMM()) {
			source = ins.readIMM();
		}

		if (RD == errors || RD == empty || RS == errors || RS == empty) {
			status = VM_Status::Error;
		}
		else
		{
			auto RDNum = regi[RD];
			auto RSNum = regi[RS];
			auto rs = reg[RSNum];

			if (op == "add") {
				uint64_t sum = (uint64_t)rs + (uint64_t)source;
				if ((~(rs ^ source) & (rs ^ sum)) & 0x80000000)		// check overflow
					status = VM_Status::Error;				// error if overflowed
				else {
					reg[RDNum] = rs + source;
					pc++;
				}
			}
			else if (op == "addu") {
				uint64_t temp = (uint64_t)rs + (uint64_t)source;
				if (temp > 4294967295) {		// if overflowed, equal the max va
					reg[RDNum] = 4294967295;
				}
				else {								// otherwise just add
					reg[RDNum] = temp;
				}
				pc++;

			}
			else if (op == "sub") {	/////////////////////////////////////////   need to find a way to check overflow

				int64_t sub = (int32_t)rs - (int32_t)source;

				if (sub < (long int)-2147483648 || sub >(long int)2147483647) {
					status = VM_Status::Error;
				}
				else {
					reg[RDNum] = (uint32_t)sub;
					pc++;
				}

			}
			else if (op == "subu") {
				uint64_t temp = rs - source;
				if (temp > 4294967295) {		// if overflowed, equal the max va
					reg[RDNum] = 4294967295;
				}
				else {								// otherwise just add
					reg[RDNum] = rs - source;
				}
				pc++;

			}
			else if (op == "and") {
				reg[RDNum] = rs & source;
				pc++;
			}
			else if (op == "nor") {
				reg[RDNum] = ~(rs | source);
				pc++;
			}
			else if (op == "or") {
				reg[RDNum] = rs | source;
				pc++;
			}
			else if (op == "xor") {
				reg[RDNum] = rs ^ source;
				pc++;
			}
		}
	}

	else if (op == "mult" || op == "multu" || op == "div" || op == "divu") {
		auto RD = ins.readRD();
		auto RS = ins.readRS();
		if (RD == errors || RD == empty || RS == errors || RS == empty) {
			status = VM_Status::Error;
		}
		else
		{
			auto RDNum = regi[RD];
			auto RSNum = regi[RS];
			int32_t rs = reg[RSNum];
			int32_t rd = reg[RDNum];

			if (op == "mult") {

				int64_t temp = (int64_t)rd * (int64_t)rs;
				if (temp >= std::numeric_limits<int32_t>::min() && temp <= std::numeric_limits<int32_t>::max()) {
					int64_t ans = (int32_t)rd * (int32_t)rs;
					lo = (uint32_t)(ans & 0xFFFFFFFFLL);
					ans >>= 32;
					hi = (uint32_t)(ans & 0xFFFFFFFFLL);
				}
				else
				{
					lo = temp & 0xFFFFFFFFLL;
					temp >>= 32;
					hi = temp & 0xFFFFFFFFLL;
				}
				pc++;
			}
			else if (op == "multu") {

				uint64_t ans = rd * rs;
				lo = ans & 0xFFFFFFFFLL;
				ans >>= 32;
				hi = ans & 0xFFFFFFFFLL;
				pc++;
			}
			else if (op == "div") {
				if (rs != 0) {
					lo = (uint32_t)((long int)rd / (long int)rs);
					hi = (uint32_t)((long int)rd % (long int)rs);
				}

				pc++;
			}
			else if (op == "divu") {
				if (rs != 0) {
					lo = rd / rs;
					hi = rd % rs;
				}

				pc++;
			}
		}
	}


	else if (op == "not") {
		auto RD = ins.readRD();
		uint32_t source = 0;
		if (ins.readURS() == true) {
			auto RS = ins.readRS();
			auto RSNum = regi[RS];
			source = reg[RSNum];
		}
		else if (ins.readUIMM()) {
			source = ins.readIMM();
		}
		else
			status = VM_Status::Error;

		if (RD == errors || RD == empty) {
			status = VM_Status::Error;
		}
		else
		{
			auto RDNum = regi[RD];
			reg[RDNum] = ~source;
			pc++;
		}
	}

	else if (op == "j") {
		auto label = ins.readMEMREF();
		pc = label;

	}
	else if (op[0] == 'b') {		// only branch instructions starts with 'b'
		auto RS = ins.readRS();
		auto RT = ins.readRT();
		if (RS == errors || RS == empty || RT == errors || RT == empty) {
			status = VM_Status::Error;
		}
		else {
			auto RSNum = regi[RS];
			auto RTNum = regi[RT];
			auto v1 = reg[RSNum];
			auto v2 = reg[RTNum];
			auto label = ins.readMEMREF();

			if (op == "beq") {
				if (v1 == v2)
					pc = label;
				else
					pc++;
			}
			else if (op == "bne") {
				if (v1 != v2)
					pc = label;
				else
					pc++;
			}
			else if (op == "blt") {
				if (v1 < v2)
					pc = label;
				else
					pc++;
			}
			else if (op == "ble") {
				if (v1 <= v2)
					pc = label;
				else
					pc++;
			}
			else if (op == "bgt") {
				if (v1 > v2)
					pc = label;
				else
					pc++;
			}
			else if (op == "bge") {
				if (v1 >= v2)
					pc = label;
				else
					pc++;
			}
			else
				status = VM_Status::Error;
		}
	}


	// not required to implement under
	/*else if (op == "lh") {

	}
	else if (op == "lb") {

	}
	else if (op == "sh") {

	}
	else if (op == "sb") {

	}
	else if (op == "mul") {

	}
	else if (op == "mulo") {

	}
	else if (op == "mulou") {

	}
	else if (op == "rem") {

	}
	else if (op == "remu") {

	}
	else if (op == "mthi") {

	}
	else if (op == "mtlo") {

	}
	else if (op == "abs") {

	}
	else if (op == "neg") {

	}
	else if (op == "negu") {

	}*/
	// not required to implement above

	else {		// should never go to here
		status = VM_Status::Error;
	}
	//std::cout << "pc++" << std::endl;
}

void VirtualMachine::simulation() {
	//status = VM_Status::Simulating;

	if (!B_labelExist("main") || bLabels["main"] != 0) {
		status = VM_Status::Error;
	}

	if (status == VM_Status::Simulating) {
		if (pc >= instructions.size()) {
			status = VM_Status::Error;
			return;
		}
		Instruction ins = instructions[pc];

		calculation(ins, status);

	}
	else
		return;
}

int VirtualMachine::memSize() {
	return memory.size();
}