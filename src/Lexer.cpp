#include "Lexer.h"

namespace toy {

Lexer::Lexer(const std::string& source)
    : source_(source), position_(0), line_(0), column_(1) {}

bool Lexer::scan(std::vector<Token>& tokens, std::vector<LexError>& errors) {
    // 每轮先跳过不生成 token 的内容，再依据当前字符选择识别方式。
    // 识别函数会移动 position_；非法字符分支也必须前进，避免死循环。
    while (!atEnd()) {
        if (!skipIgnored(errors) || atEnd()) {
            break;
        }
        if (isIdentifierStart(peek())) { // 字母或下划线：关键字或标识符。
            scanIdentifier(tokens);
        } else if (isDigit(peek())) {    // 数字：连续的十进制数字串。
            scanInteger(tokens);
        } else if (!scanSymbol(tokens)) {
            // 不属于任何合法符号：记录当前位置，跳过该字符后继续扫描。
            const int line = line_;
            const int column = column_;
            const std::string text = consumeIllegalCharacter();
            std::string shown = text;
            // 特殊字符在诊断中使用可读的显示形式。
            if (text == "\\") shown = "\\\\";
            if (text == "'") shown = "\"'\"";
            errors.push_back({line, column, "illegal character '" + shown + "'"});
        }
    }
    return errors.empty();
}

bool Lexer::atEnd() const { return position_ >= source_.size(); }

// offset 用于向前查看，例如同时检查当前字符和下一个字符。
// 越过源码末尾时返回 '\0'，不改变扫描位置。
char Lexer::peek(std::size_t offset) const {
    const std::size_t index = position_ + offset;
    return index < source_.size() ? source_[index] : '\0';
}

char Lexer::advance() {
    // 此函数只在尚未到达末尾时调用；每次消费一个字节。
    const char ch = source_[position_++];
    if (ch == '\n') { ++line_; column_ = 1; } else { ++column_; }
    return ch;
}

bool Lexer::skipIgnored(std::vector<LexError>& errors) {
    // 连续跳过空白、单行注释和块注释，直到遇到实际 token 的开头。
    while (!atEnd()) {
        if (isWhitespace(peek())) { advance(); continue; }
        if (peek() == '/' && peek(1) == '/') {
            // 换行符留给下一轮按空白处理，以便统一更新行号。
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
    // 保存起始位置，便于注释没有闭合时报告错误所在行。
    const int line = line_;
    const int column = column_;
    advance(); advance(); // 消费开头的 /*。
    while (!atEnd() && !(peek() == '*' && peek(1) == '/')) advance();
    if (atEnd()) {
        errors.push_back({line, column, "unterminated block comment"});
        return false;
    }
    advance(); advance(); // 消费结尾的 */。
    return true;
}

void Lexer::scanIdentifier(std::vector<Token>& tokens) {
    // 先完整读取标识符，再与关键字表比较；因此 integer 不会被拆成 int 和 eger。
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
    // 这里只识别并保存数字原文，不在词法阶段转换数值或检查范围。
    const std::size_t start = position_;
    const int line = line_, column = column_;
    while (isDigit(peek())) advance();
    tokens.push_back({"IntConst", source_.substr(start, position_ - start), line, column});
}

bool Lexer::scanSymbol(std::vector<Token>& tokens) {
    // 先检查双字符符号，再检查单字符，确保 <= 等不会被拆开。
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
    // 根据 UTF-8 首字节估计字符宽度，将对应字节作为一个非法字符跳过。
    // 这里不验证后续字节是否合法，也不会越过输入末尾。
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
