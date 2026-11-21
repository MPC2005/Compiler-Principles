#ifndef TOYC_LEXER_H
#define TOYC_LEXER_H

#include <string>
#include <vector>

namespace toy {

// 一个词法单元：同时保留分类、源码原文和起始位置。
struct Token {
    std::string type;  // 例如 Ident、IntConst 或 'int'。
    std::string text;  // 源码中实际出现的文本。
    int line;          // 零基行号。
    int column;        // 一基列号，按源码字节推进。
};

// 词法错误的位置和说明；输出格式由调用方决定。
struct LexError {
    int line;
    int column;
    std::string message;
};

class Lexer {
public:
    // 保存对 source 的引用；调用方须保证源码在扫描期间仍然有效。
    explicit Lexer(const std::string& source);
    // 将结果追加到调用方提供的容器；errors 为空时返回 true。
    bool scan(std::vector<Token>& tokens, std::vector<LexError>& errors);

private:
    const std::string& source_;  // 输入源码，本类不拥有该字符串。
    std::size_t position_;       // 下一个待处理字节在 source_ 中的位置。
    int line_;                   // 下一个待处理字节所在行，从 0 开始。
    int column_;                 // 下一个待处理字节所在列，从 1 开始。

    // 基础读取：peek 只查看，advance 才会移动位置并更新行列。
    bool atEnd() const;
    char peek(std::size_t offset = 0) const;
    char advance();
    // 跳过空白和注释；块注释未结束时记录错误并返回 false。
    bool skipIgnored(std::vector<LexError>& errors);
    bool skipBlockComment(std::vector<LexError>& errors);
    // 分别识别标识符或关键字、十进制数字串和合法符号。
    void scanIdentifier(std::vector<Token>& tokens);
    void scanInteger(std::vector<Token>& tokens);
    bool scanSymbol(std::vector<Token>& tokens);
    // 跳过非法字符，以便扫描继续向前进行。
    std::string consumeIllegalCharacter();
    // 字符分类仅按 ToyC 当前支持的 ASCII 规则进行。
    static bool isLetter(char ch);
    static bool isDigit(char ch);
    static bool isIdentifierStart(char ch);
    static bool isIdentifierPart(char ch);
    static bool isWhitespace(char ch);
};

}  // namespace toy

#endif
