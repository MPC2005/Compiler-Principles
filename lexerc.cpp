#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>

using namespace std;

enum class TokenType {
    KW_INT, KW_CONST, KW_VOID, 
    KW_IF, KW_ELSE, KW_WHILE, KW_RETURN, KW_BREAK, KW_CONTINUE,
    IDENT, INT_CONST,
    PLUS, MINUS, STAR, SLASH, PERCENT, ASSIGN,
    EQ, NEQ, LT, GT, LEQ, GEQ,
    LOGIC_AND, LOGIC_OR, NOT,
    LPAREN, RPAREN, LBRACE, RBRACE,
    SEMICOLON, COMMA,
    INVALID
};

struct Token {
    TokenType type;
    string lexeme;
    int line;
    int column;
    
    Token(TokenType t, string lex, int l, int c) 
        : type(t), lexeme(lex), line(l), column(c) {}
};

TokenType getKeywordType(const string& word) {
    if (word == "int") return TokenType::KW_INT;
    if (word == "const") return TokenType::KW_CONST;
    if (word == "void") return TokenType::KW_VOID;
    if (word == "if") return TokenType::KW_IF;
    if (word == "else") return TokenType::KW_ELSE;
    if (word == "while") return TokenType::KW_WHILE;
    if (word == "return") return TokenType::KW_RETURN;
    if (word == "break") return TokenType::KW_BREAK;
    if (word == "continue") return TokenType::KW_CONTINUE;
    return TokenType::IDENT;
}

TokenType getOpType(const string& op) {
    if (op == "+") return TokenType::PLUS;
    if (op == "-") return TokenType::MINUS;
    if (op == "*") return TokenType::STAR;
    if (op == "/") return TokenType::SLASH;
    if (op == "%") return TokenType::PERCENT;
    if (op == "=") return TokenType::ASSIGN;
    if (op == "==") return TokenType::EQ;
    if (op == "!=") return TokenType::NEQ;
    if (op == "<") return TokenType::LT;
    if (op == ">") return TokenType::GT;
    if (op == "<=") return TokenType::LEQ;
    if (op == ">=") return TokenType::GEQ;
    if (op == "&&") return TokenType::LOGIC_AND;
    if (op == "||") return TokenType::LOGIC_OR;
    if (op == "!") return TokenType::NOT;
    if (op == "(") return TokenType::LPAREN;
    if (op == ")") return TokenType::RPAREN;
    if (op == "{") return TokenType::LBRACE;
    if (op == "}") return TokenType::RBRACE;
    if (op == ";") return TokenType::SEMICOLON;
    if (op == ",") return TokenType::COMMA;
    return TokenType::INVALID;
}

string getTypeString(TokenType type, const string& lexeme) {
    if (type == TokenType::IDENT) return "Ident";
    if (type == TokenType::INT_CONST) return "IntConst";
    return "'" + lexeme + "'";
}

bool isIdentStart(char c) {
    return c == '_' || isalpha(static_cast<unsigned char>(c));
}

bool isIdentChar(char c) {
    return isIdentStart(c) || isdigit(static_cast<unsigned char>(c));
}

bool isSingleOp(char c) {
    string ops = "+-*/%!=><(){};,";
    return ops.find(c) != string::npos;
}

int utf8Len(unsigned char b) {
    if ((b & 0x80) == 0) return 1;
    if ((b & 0xE0) == 0xC0) return 2;
    if ((b & 0xF0) == 0xE0) return 3;
    if ((b & 0xF8) == 0xF0) return 4;
    return 1;
}

string formatError(int line, const string& ch) {
    string quotedCh = (ch == "'") ? "\"'\"" : "'" + ch + "'";
    return to_string(line) + " illegal character " + quotedCh;
}

int main() {
    string str;
    char ch;
    while (cin.get(ch)) {
        str += ch;
    }

    int n = (int)str.size();
    int line = 0; 
    int column = 0; 
    
    vector<Token> tokens;
    vector<string> errors;

    for (int i = 0; i < n; ) {
        char c = str[i];

        if (c == '\n') {
            ++line;
            column = 0;
            ++i;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            ++column;
            ++i;
            continue;
        }

        if (c == '/' && i + 1 < n && str[i + 1] == '/') {
            i += 2;
            column += 2;
            while (i < n && str[i] != '\n') {
                ++i; ++column;
            }
            continue;
        }

        if (c == '/' && i + 1 < n && str[i + 1] == '*') {
            i += 2;
            column += 2;
            while (i + 1 < n && !(str[i] == '*' && str[i + 1] == '/')) {
                if (str[i] == '\n') {
                    ++line;
                    column = 0;
                } else {
                    ++column;
                }
                ++i;
            }
            if (i + 1 < n) {
                i += 2;
                column += 2;
            } else {
                i = n;
            }
            continue;
        }

        if (isIdentStart(c)) {
            int startCol = column;
            int j = i + 1;
            while (j < n && isIdentChar(str[j])) ++j;
            
            string word = str.substr(i, j - i);
            TokenType type = getKeywordType(word);
            tokens.emplace_back(type, word, line, startCol);
            
            column += (j - i);
            i = j;
            continue;
        }

        if (isdigit(static_cast<unsigned char>(c))) {
            int startCol = column;
            int j = i;
            while (j < n && isIdentChar(str[j])) ++j;
            
            string word = str.substr(i, j - i);
            tokens.emplace_back(TokenType::INT_CONST, word, line, startCol);
            
            column += (j - i);
            i = j;
            continue;
        }

        if (i + 1 < n) {
            string two = str.substr(i, 2);
            if (two == "==" || two == "!=" || two == ">=" || 
                two == "<=" || two == "&&" || two == "||") {
                int startCol = column;
                tokens.emplace_back(getOpType(two), two, line, startCol);
                column += 2;
                i += 2;
                continue;
            }
        }

        if (isSingleOp(c)) {
            string one(1, c);
            tokens.emplace_back(getOpType(one), one, line, column);
            ++column;
            ++i;
            continue;
        }

        if ((unsigned char)c >= 0x80) {
            int len = utf8Len((unsigned char)c);
            if (i + len > n) len = n - i;
            string chstr = str.substr(i, len);
            
            errors.push_back(formatError(line, chstr));
            column += len;
            i += len;
            continue;
        }

        errors.push_back(formatError(line, string(1, c)));
        ++column;
        ++i;
    }

    if (!errors.empty()) {
        cout << "reject\n";
        for (const auto& e : errors) {
            cout << e << "\n";
        }
    }

    for (int k = 0; k < (int)tokens.size(); ++k) {
        const auto& tk = tokens[k];
        cout << k << ":" << getTypeString(tk.type, tk.lexeme) 
             << ":\"" << tk.lexeme << "\"\n";
    }

    return 0;
}