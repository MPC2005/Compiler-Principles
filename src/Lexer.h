#ifndef TOYC_LEXER_H
#define TOYC_LEXER_H

#include <string>
#include <vector>

namespace toy {

struct Token {
    std::string type;
    std::string text;
    int line;
    int column;
};

struct LexError {
    int line;
    int column;
    std::string message;
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    bool scan(std::vector<Token>& tokens, std::vector<LexError>& errors);

private:
    const std::string& source_;
    std::size_t position_;
    int line_;
    int column_;

    bool atEnd() const;
    char peek(std::size_t offset = 0) const;
    char advance();
    bool skipIgnored(std::vector<LexError>& errors);
    bool skipBlockComment(std::vector<LexError>& errors);
    void scanIdentifier(std::vector<Token>& tokens);
    void scanInteger(std::vector<Token>& tokens);
    bool scanSymbol(std::vector<Token>& tokens);
    std::string consumeIllegalCharacter();
    static bool isLetter(char ch);
    static bool isDigit(char ch);
    static bool isIdentifierStart(char ch);
    static bool isIdentifierPart(char ch);
    static bool isWhitespace(char ch);
};

}  // namespace toy

#endif
