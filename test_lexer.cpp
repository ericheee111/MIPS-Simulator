#include "catch.hpp"

#include "lexer.hpp"

#include <string>
#include <sstream>
#include <map>


TEST_CASE("test empty stream", "[lexer]") {

    {
        std::string input = "";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() == 0);
    }

    {
        std::string input = "    \t    \r      ";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() == 0);
    }
}

TEST_CASE("test simple strings, one per line", "[lexer]") {

    {  //ends with newline
        std::string input = "this\nis\na\nsequence\nof\nlines\n";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() == 12);
        REQUIRE(tl.back() != Token(ERROR, 0));

        TokenList::iterator it = tl.begin();
        REQUIRE(*it == Token(STRING, 1, "this"));
        REQUIRE(*(++it) == Token(EOL, 1));
        REQUIRE(*(++it) == Token(STRING, 2, "is"));
        REQUIRE(*(++it) == Token(EOL, 2));
        REQUIRE(*(++it) == Token(STRING, 3, "a"));
        REQUIRE(*(++it) == Token(EOL, 3));
        REQUIRE(*(++it) == Token(STRING, 4, "sequence"));
        REQUIRE(*(++it) == Token(EOL, 4));
        REQUIRE(*(++it) == Token(STRING, 5, "of"));
        REQUIRE(*(++it) == Token(EOL, 5));
        REQUIRE(*(++it) == Token(STRING, 6, "lines"));
        REQUIRE(*(++it) == Token(EOL, 6));
    }

    {  //does not end with newline
        std::string input = "this\nis\na\nsequence\nof\nlines";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() == 12);
        REQUIRE(tl.back() != Token(ERROR, 0));

        TokenList::iterator it = tl.begin();
        REQUIRE(*it == Token(STRING, 1, "this"));
        REQUIRE(*(++it) == Token(EOL, 1));
        REQUIRE(*(++it) == Token(STRING, 2, "is"));
        REQUIRE(*(++it) == Token(EOL, 2));
        REQUIRE(*(++it) == Token(STRING, 3, "a"));
        REQUIRE(*(++it) == Token(EOL, 3));
        REQUIRE(*(++it) == Token(STRING, 4, "sequence"));
        REQUIRE(*(++it) == Token(EOL, 4));
        REQUIRE(*(++it) == Token(STRING, 5, "of"));
        REQUIRE(*(++it) == Token(EOL, 5));
        REQUIRE(*(++it) == Token(STRING, 6, "lines"));
        REQUIRE(*(++it) == Token(EOL, 6));
    }
}

TEST_CASE("test lists", "[lexer]") {
    {
        std::string input = "this,is, a \t , list , of, strings";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() == 12);
        REQUIRE(tl.back() != Token(ERROR, 0));

        TokenList::iterator it = tl.begin();
        REQUIRE(*it == Token(STRING, 1, "this"));
        REQUIRE(*(++it) == Token(SEP, 1));
        REQUIRE(*(++it) == Token(STRING, 1, "is"));
        REQUIRE(*(++it) == Token(SEP, 1));
        REQUIRE(*(++it) == Token(STRING, 1, "a"));
        REQUIRE(*(++it) == Token(SEP, 1));
        REQUIRE(*(++it) == Token(STRING, 1, "list"));
        REQUIRE(*(++it) == Token(SEP, 1));
        REQUIRE(*(++it) == Token(STRING, 1, "of"));
        REQUIRE(*(++it) == Token(SEP, 1));
        REQUIRE(*(++it) == Token(STRING, 1, "strings"));
        REQUIRE(*(++it) == Token(EOL, 1));
    }

    {
        std::string input = "this, is another, list";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() == 7);
        REQUIRE(tl.back() != Token(ERROR, 0));

        TokenList::iterator it = tl.begin();
        REQUIRE(*it == Token(STRING, 1, "this"));
        REQUIRE(*(++it) == Token(SEP, 1));
        REQUIRE(*(++it) == Token(STRING, 1, "is"));
        REQUIRE(*(++it) == Token(STRING, 1, "another"));
        REQUIRE(*(++it) == Token(SEP, 1));
        REQUIRE(*(++it) == Token(STRING, 1, "list"));
        REQUIRE(*(++it) == Token(EOL, 1));
    }
}

TEST_CASE("test parens", "[lexer]") {

    std::string input = "outside(inside inside )(inside)outside";
    std::istringstream iss(input);

    TokenList tl = tokenize(iss);

    REQUIRE(tl.size() == 10);
    REQUIRE(tl.back() != Token(ERROR, 0));

    TokenList::iterator it = tl.begin();
    REQUIRE(*it == Token(STRING, 1, "outside"));
    REQUIRE(*(++it) == Token(OPEN_PAREN, 1));
    REQUIRE(*(++it) == Token(STRING, 1, "inside"));
    REQUIRE(*(++it) == Token(STRING, 1, "inside"));
    REQUIRE(*(++it) == Token(CLOSE_PAREN, 1));
    REQUIRE(*(++it) == Token(OPEN_PAREN, 1));
    REQUIRE(*(++it) == Token(STRING, 1, "inside"));
    REQUIRE(*(++it) == Token(CLOSE_PAREN, 1));
    REQUIRE(*(++it) == Token(STRING, 1, "outside"));
    REQUIRE(*(++it) == Token(EOL, 1));
}

TEST_CASE("test delimited strings", "[lexer]") {

    std::string input = " a \"b c d\" e \"\" f";
    std::istringstream iss(input);

    TokenList tl = tokenize(iss);

    REQUIRE(tl.size() == 10);
    REQUIRE(tl.back() != Token(ERROR, 0));

    TokenList::iterator it = tl.begin();
    REQUIRE(*it == Token(STRING, 1, "a"));
    REQUIRE(*(++it) == Token(STRING_DELIM, 1));
    REQUIRE(*(++it) == Token(STRING, 1, "b c d"));
    REQUIRE(*(++it) == Token(STRING_DELIM, 1));
    REQUIRE(*(++it) == Token(STRING, 1, "e"));
    REQUIRE(*(++it) == Token(STRING_DELIM, 1));
    REQUIRE(*(++it) == Token(STRING, 1, ""));
    REQUIRE(*(++it) == Token(STRING_DELIM, 1));
    REQUIRE(*(++it) == Token(STRING, 1, "f"));
    REQUIRE(*(++it) == Token(EOL, 1));
}

TEST_CASE("test equal token", "[lexer]") {

    std::string input = R"(
  .data
  NAME1 = 1
  NAME2 = 2
  NAME3 = -3)";

    std::istringstream iss(input);
    TokenList tl = tokenize(iss);

    REQUIRE(tl.size() == 14);
    REQUIRE(tl.back() != Token(ERROR, 0));

    TokenList::iterator it = tl.begin();
    REQUIRE(*it == Token(STRING, 2, ".data"));
    REQUIRE(*(++it) == Token(EOL, 2));
    REQUIRE(*(++it) == Token(STRING, 3, "NAME1"));
    REQUIRE(*(++it) == Token(EQUAL, 3));
    REQUIRE(*(++it) == Token(STRING, 3, "1"));
    REQUIRE(*(++it) == Token(EOL, 3));
    REQUIRE(*(++it) == Token(STRING, 4, "NAME2"));
    REQUIRE(*(++it) == Token(EQUAL, 4));
    REQUIRE(*(++it) == Token(STRING, 4, "2"));
    REQUIRE(*(++it) == Token(EOL, 4));
    REQUIRE(*(++it) == Token(STRING, 5, "NAME3"));
    REQUIRE(*(++it) == Token(EQUAL, 5));
    REQUIRE(*(++it) == Token(STRING, 5, "-3"));
    REQUIRE(*(++it) == Token(EOL, 5));
}

TEST_CASE("test error handling and reporting", "[lexer]") {

    {
        std::string input = "\n\n\nfoo \"bar\n";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() > 0);
        REQUIRE(tl.back().type() == ERROR);
        REQUIRE(tl.back().value().substr(0, 7) == "Error: ");
    }

    {
        std::string input = "foo (bar\n";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() > 0);
        REQUIRE(tl.back().type() == ERROR);
        REQUIRE(tl.back().value().substr(0, 7) == "Error: ");
    }

    {
        std::string input = "foo )bar\n";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() > 0);
        REQUIRE(tl.back().type() == ERROR);
        REQUIRE(tl.back().value().substr(0, 7) == "Error: ");
    }

    {
        std::string input = "foo ((bar)\n";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() > 0);
        REQUIRE(tl.back().type() == ERROR);
        REQUIRE(tl.back().value().substr(0, 7) == "Error: ");
    }

    {
        std::string input = "foo (bar) baz)\n";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() > 0);
        REQUIRE(tl.back().type() == ERROR);
        REQUIRE(tl.back().value().substr(0, 7) == "Error: ");
    }

    {
        std::string input = "\nfoo (\"bar)\" baz\n";
        std::istringstream iss(input);

        TokenList tl = tokenize(iss);

        REQUIRE(tl.size() > 0);
        REQUIRE(tl.back().type() == ERROR);
        REQUIRE(tl.back().value().substr(0, 7) == "Error: ");
    }
}



TEST_CASE("xxxxx", "[lexer]") {

    std::string input = R"(
  .data
var6:	.ascii "hello"

var7:	.asciiz "goodbye"

	.text
)";

    std::istringstream iss(input);
    TokenList tl = tokenize(iss);

    REQUIRE(tl.size() == 16);
    REQUIRE(tl.back() != Token(ERROR, 0));

    TokenList::iterator it = tl.begin();
    REQUIRE(*it == Token(STRING, 2, ".data"));
    REQUIRE(*(++it) == Token(EOL, 2));
    REQUIRE(*(++it) == Token(STRING, 3, "var6:"));
    REQUIRE(*(++it) == Token(STRING, 3, ".ascii"));
    REQUIRE(*(++it) == Token(STRING_DELIM, 3));
    REQUIRE(*(++it) == Token(STRING, 3, "hello"));
    REQUIRE(*(++it) == Token(STRING_DELIM, 3));
    REQUIRE(*(++it) == Token(EOL, 3));
    REQUIRE(*(++it) == Token(STRING, 5, "var7:"));
    REQUIRE(*(++it) == Token(STRING, 5, ".asciiz"));
    REQUIRE(*(++it) == Token(STRING_DELIM, 5));
    REQUIRE(*(++it) == Token(STRING, 5, "goodbye"));
    REQUIRE(*(++it) == Token(STRING_DELIM, 5));
    REQUIRE(*(++it) == Token(EOL, 5));
    REQUIRE(*(++it) == Token(STRING, 7, ".text"));
    REQUIRE(*(++it) == Token(EOL, 7));
}

TEST_CASE("xxxxxxxx", "[lexer]") {

    std::string input = R"(     	
	.data
var1:	.word 1024             

var2:	.half 12               

	.text
	

)";


    std::istringstream iss(input);
    TokenList tl = tokenize(iss);

    REQUIRE(tl.size() == 12);
    REQUIRE(tl.back() != Token(ERROR, 0));

    TokenList::iterator it = tl.begin();
    REQUIRE(*it == Token(STRING, 2, ".data"));
    REQUIRE(*(++it) == Token(EOL, 2));
    REQUIRE(*(++it) == Token(STRING, 3, "var1:"));
    REQUIRE(*(++it) == Token(STRING, 3, ".word"));
    REQUIRE(*(++it) == Token(STRING, 3, "1024"));
    REQUIRE(*(++it) == Token(EOL, 3));
    REQUIRE(*(++it) == Token(STRING, 5, "var2:"));
    REQUIRE(*(++it) == Token(STRING, 5, ".half"));
    REQUIRE(*(++it) == Token(STRING, 5, "12"));
    REQUIRE(*(++it) == Token(EOL, 5));
    REQUIRE(*(++it) == Token(STRING, 7, ".text"));
    REQUIRE(*(++it) == Token(EOL, 7));

}