#ifndef LEXER_HPP
#define LEXER_HPP

#include <istream>

#include "token.hpp"

/* Tokenize input text string into a list of tokens.
On error, the last token is of type ERROR and the value is the
error message.
 */
TokenList tokenize(std::istream & ins);

#endif
