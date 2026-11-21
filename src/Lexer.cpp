#include "Lexer.h"

namespace toy {

Lexer::Lexer(const std::string& source)
    : source_(source), position_(0), line_(0), column_(1) {}

bool Lexer::scan(std::vector<Token>& tokens, std::vector<LexError>& errors) {
    while (!atEnd()) {
        if (!skipIgnored(errors) || atEnd()) {
            break;
        }
        if (isIdentifierStart(peek())) {
            scanIdentifier(tokens);
        } else if (isDigit(peek())) {
            scanInteger(tokens);
        } else if (!scanSymbol(tokens)) {
            const int line = line_;
            const int column = column_;
            const std::string text = consumeIllegalCharacter();
            std::string shown = text;
            if (text == "\\") shown = "\\\\";
            if (text == "'") shown = "\"'\"";
            errors.push_back({line, column, "illegal character '" + shown + "'"});
        }
    }
    return errors.empty();
}

bool Lexer::atEnd() const { return position_ >= source_.size(); }

char Lexer::peek(std::size_t offset) const {
    const std::size_t index = position_ + offset;
    return index < source_.size() ? source_[index] : '\0';
}

char Lexer::advance() {
    const char ch = source_[position_++];
    if (ch == '\n') { ++line_; column_ = 1; } else { ++column_; }
    return ch;
}

bool Lexer::skipIgnored(std::vector<LexError>& errors) {
    while (!atEnd()) {
        if (isWhitespace(peek())) { advance(); continue; }
        if (peek() == '/' && peek(1) == '/') {
            while (!atEnd() && peek() != '\n') advance();
            continue;
        }
        if (peek() == '/' && peek(1) == '*') {
            if (!skipBlockComment(errors)) return false;
            continue;
        }
        break;
    }
    return true;
}

bool Lexer::skipBlockComment(std::vector<LexError>& errors) {
    const int line = line_;
    const int column = column_;
    advance(); advance();
    while (!atEnd() && !(peek() == '*' && peek(1) == '/')) advance();
    if (atEnd()) {
        errors.push_back({line, column, "unterminated block comment"});
        return false;
    }
    advance(); advance();
    return true;
}

void Lexer::scanIdentifier(std::vector<Token>& tokens) {
    const std::size_t start = position_;
    const int line = line_, column = column_;
    advance();
    while (isIdentifierPart(peek())) advance();
    const std::string text = source_.substr(start, position_ - start);
    static const char* const keywords[] = {"int", "void", "const", "if", "else", "while", "break", "continue", "return"};
    std::string type = "Ident";
    for (const char* keyword : keywords) {
        if (text == keyword) { type = "'" + text + "'"; break; }
    }
    tokens.push_back({type, text, line, column});
}

void Lexer::scanInteger(std::vector<Token>& tokens) {
    const std::size_t start = position_;
    const int line = line_, column = column_;
    while (isDigit(peek())) advance();
    tokens.push_back({"IntConst", source_.substr(start, position_ - start), line, column});
}

bool Lexer::scanSymbol(std::vector<Token>& tokens) {
    static const char* const two[] = {"&&", "||", "<=", ">=", "==", "!="};
    for (const char* value : two) {
        const std::string spelling(value);
        if (source_.compare(position_, spelling.size(), spelling) == 0) {
            tokens.push_back({"'" + spelling + "'", spelling, line_, column_});
            advance(); advance();
            return true;
        }
    }
    static const std::string one = "+-!*/%<>=;,(){}";
    if (one.find(peek()) == std::string::npos) return false;
    const std::string spelling(1, peek());
    tokens.push_back({"'" + spelling + "'", spelling, line_, column_});
    advance();
    return true;
}

std::string Lexer::consumeIllegalCharacter() {
    const unsigned char first = static_cast<unsigned char>(peek());
    std::size_t width = 1;
    if ((first & 0xE0) == 0xC0) width = 2;
    else if ((first & 0xF0) == 0xE0) width = 3;
    else if ((first & 0xF8) == 0xF0) width = 4;
    std::string text;
    for (std::size_t i = 0; i < width && !atEnd(); ++i) text.push_back(advance());
    return text;
}

bool Lexer::isLetter(char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }
bool Lexer::isDigit(char ch) { return ch >= '0' && ch <= '9'; }
bool Lexer::isIdentifierStart(char ch) { return isLetter(ch) || ch == '_'; }
bool Lexer::isIdentifierPart(char ch) { return isIdentifierStart(ch) || isDigit(ch); }
bool Lexer::isWhitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v'; }

}  // namespace toy
