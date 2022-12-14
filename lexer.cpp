#include "lexer.hpp"
#include <iostream>
using namespace std;

// implement the tokenize function here
// currently it just always returns an empty TokenList
TokenList tokenize(std::istream& ins) {
	TokenList tokens;

	bool in_paren = false;
	bool error = false;
	bool enddelim = false;
	char c;
	string temp;
	auto line = 1;

	while (ins.get(c)) {
		switch (c) {
		case '\n':
			if (!temp.empty() && in_paren == false && error == false) {
				tokens.emplace_back(STRING, line, temp);
				temp.clear();
				tokens.emplace_back(EOL, line);
			}
			else if (enddelim == true && in_paren == false && error == false) {
				enddelim = false;
				tokens.emplace_back(EOL, line);
			}
			/*else if (in_paren == false && error == false) {
				tokens.emplace_back(EOL, line);
			}*/

			else if (in_paren)
				tokens.emplace_back(ERROR, line, "Error: parenthesis can't have multiple lines.");

			line++;
			break;
		case ',':
			if (!temp.empty()) {
				tokens.emplace_back(STRING, line, temp);

				temp.clear();
			}
			tokens.emplace_back(SEP, line);
			break;
		case '=':
			if (!temp.empty()) {
				tokens.emplace_back(STRING, line, temp);
				temp.clear();
			}
			tokens.emplace_back(EQUAL, line);
			break;

		case '(':
			if (in_paren == false) {
				if (!temp.empty()) {
					tokens.emplace_back(STRING, line, temp);
					temp.clear();
				}
				tokens.emplace_back(OPEN_PAREN, line);

				in_paren = true;
			}
			else {
				tokens.emplace_back(ERROR, line, "Error: already in parenthesis.");
				error = true;
			}


			break;
		case ')':
			if (in_paren == true && error == false) {
				if (!temp.empty()) {
					tokens.emplace_back(STRING, line, temp);
					temp.clear();
				}
				tokens.emplace_back(CLOSE_PAREN, line);
				in_paren = false;
			}
			else if (in_paren == false) {
				tokens.emplace_back(ERROR, line, "Error: not in open parenthesis.");
				error = true;
			}
			break;
		case '"':
			tokens.emplace_back(STRING_DELIM, line);
			do {
				ins.get(c);
				if (c == '"') break;

				else if (c == '\n') {
					tokens.emplace_back(ERROR, line, "Error: string can't span multiple lines.");
					error = true;
					break;
				}
				temp.push_back(c);
			} while (c != '"' && ins.peek() != EOF);
			if (error == false) {
				tokens.emplace_back(STRING, line, temp);
				temp.clear();
				tokens.emplace_back(STRING_DELIM, line);
				enddelim = true;
			}
			break;
		case '#':
			do {
				ins.get(c);
			} while (c != '\n' && ins.peek() != EOF);
			tokens.emplace_back(EOL, line);
			break;
		case ' ':
		case '\r':
		case '\t':
		case '\v':
		case '\f':
			if (!temp.empty()) {
				tokens.emplace_back(STRING, line, temp);
				temp.clear();
				enddelim = true;
			}
			break;


		default: temp.push_back(c);
		}
	}
	if (!temp.empty() && error == false && in_paren == false) {
		tokens.emplace_back(STRING, line, temp);
		temp.clear();
		tokens.emplace_back(EOL, line);
		line++;
	}
	return TokenList(tokens);
}
