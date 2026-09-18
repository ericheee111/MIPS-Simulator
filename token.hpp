#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstddef>
#include <list>
#include <ostream>
#include <string>

/* A token is an end-of-line, list element seperator (,),
constant assignment operator (=), open/close parenthesis,
string delimiter ("), or a string.
ERROR is used to report lexing errors
*/
enum TokenType {
  EOL,
  SEP,
  EQUAL,
  OPEN_PAREN,
  CLOSE_PAREN,
  STRING_DELIM,
  STRING,
  ERROR
};

/* A token has a type, value, and records the original
source line on which it appears.
 */
class Token {
public:
  // construct a token type on line with empty value
  Token(TokenType type, std::size_t line);

  // construct a token type on line  with value
  Token(TokenType type, std::size_t line, const std::string &value);

  // return the token type
  TokenType type() const;

  // return the token's originating source line
  std::size_t line() const;

  // Return an owning value, preserving the original public API and making
  // references bound to Token(...).value() safe after the Token is destroyed.
  std::string value() const;

  // Borrow without a copy from a live lvalue Token only. The reference expires
  // when that Token is changed or destroyed; rvalue borrowing is forbidden.
  const std::string& valueRef() const & noexcept;
  const std::string& valueRef() const && = delete;

private:
  TokenType m_type;
  std::size_t m_line;
  std::string m_value;
};

// comparison operators for tokens, every field must match
bool operator==(const Token &t1, const Token &t2);
bool operator!=(const Token &t1, const Token &t2);

// convienience function for printing tokens
// this makes the Catch error reports more informative
std::ostream &operator<<(std::ostream &oss, const Token &t);

// The token sequence is defined as a std::list of tokens.
typedef std::list<Token> TokenList;

#endif
