#include "lexer.hpp"
#include "mips/numbers.hpp"

TokenList tokenize(std::istream& input) {
    TokenList tokens;
    std::string word;
    std::size_t line = 1, bytes = 0;
    bool string = false, paren = false, comment = false, nonempty = false;
    auto emitWord = [&] {
        if (!word.empty()) { tokens.emplace_back(STRING, line, word); word.clear(); nonempty = true; }
    };
    auto fail = [&](const std::string& message) {
        tokens.emplace_back(ERROR, line, "Error: " + message);
    };
    auto endLine = [&] {
        emitWord();
        if (nonempty) tokens.emplace_back(EOL, line);
        nonempty = false;
    };
    char c;
    while (input.get(c)) {
        if (++bytes > mips::MaxSourceBytes) { fail("source exceeds 4 MiB limit"); return tokens; }
        if (comment && c != '\n') continue;
        if (c == '\n') {
            if (string || paren) { fail("string or parenthesis cannot span lines"); return tokens; }
            endLine();
            comment = false;
            ++line;
            continue;
        }
        const auto byte = static_cast<unsigned char>(c);
        if (string) {
            if (c == '"') {
                tokens.emplace_back(STRING, line, word); // Includes the empty literal.
                tokens.emplace_back(STRING_DELIM, line);
                word.clear(); string = false;
            } else {
                if (byte < 32 || byte > 126) { fail("string must contain printable ASCII"); return tokens; }
                word.push_back(c);
            }
            continue;
        }
        if (c == '#') { emitWord(); comment = true; continue; }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') { emitWord(); continue; }
        if (byte < 32 || byte > 126) { fail("unexpected non-ASCII source character"); return tokens; }
        switch (c) {
        case '"': emitWord(); tokens.emplace_back(STRING_DELIM, line); string = true; nonempty = true; break;
        case '(':
            if (paren) { fail("nested parenthesis"); return tokens; }
            emitWord(); tokens.emplace_back(OPEN_PAREN, line); paren = true; nonempty = true; break;
        case ')':
            if (!paren) { fail("unmatched closing parenthesis"); return tokens; }
            emitWord(); tokens.emplace_back(CLOSE_PAREN, line); paren = false; nonempty = true; break;
        case ',': emitWord(); tokens.emplace_back(SEP, line); nonempty = true; break;
        case '=': emitWord(); tokens.emplace_back(EQUAL, line); nonempty = true; break;
        default: word.push_back(c); break;
        }
    }
    if (input.bad()) { fail("input read failed"); return tokens; }
    if (string || paren) { fail("unterminated string or parenthesis at EOF"); return tokens; }
    endLine(); // EOF after spaces and EOF immediately after a token are equivalent.
    return tokens;
}
