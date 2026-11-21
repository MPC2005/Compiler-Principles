#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stack>
#include <map>
#include <memory>
#include <algorithm>
#include <climits>
#include "LL1TableData.h"

using namespace std;

// ==========================================================
// 第一部分：词法分析器 (Lexer)
// ==========================================================

enum class TokenType {
    KW_INT, KW_CONST, KW_VOID, KW_IF, KW_ELSE, KW_WHILE, KW_RETURN, KW_BREAK, KW_CONTINUE,
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

    Token(TokenType t, string lex, int l) : type(t), lexeme(lex), line(l) {}
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

bool isIdentStart(char c) { return c == '_' || isalpha(static_cast<unsigned char>(c)); }
bool isIdentChar(char c) { return isIdentStart(c) || isdigit(static_cast<unsigned char>(c)); }
bool isSingleOp(char c) { return string("+-*/%!=><(){};,").find(c) != string::npos; }
bool isDigitChar(char c) { return c >= '0' && c <= '9'; }

int utf8Len(unsigned char b) {
    if ((b & 0x80) == 0) return 1;
    if ((b & 0xE0) == 0xC0) return 2;
    if ((b & 0xF0) == 0xE0) return 3;
    if ((b & 0xF8) == 0xF0) return 4;
    return 1;
}

// 词法分析主函数：错误只记录 0-based 行号
void runLexer(const string& str, vector<Token>& tokens, vector<int>& lexErrors) {
    int n = (int)str.size();
    int line = 0;

    for (int i = 0; i < n; ) {
        char c = str[i];

        if (c == '\n') { ++line; ++i; continue; }
        if (c == ' ' || c == '\t' || c == '\r') { ++i; continue; }

        // 单行注释
        if (c == '/' && i + 1 < n && str[i + 1] == '/') {
            i += 2;
            while (i < n && str[i] != '\n') ++i;
            continue;
        }
        // 多行注释：未闭合时在注释起始行报错
        if (c == '/' && i + 1 < n && str[i + 1] == '*') {
            int startLine = line;
            i += 2;
            bool closed = false;
            while (i < n) {
                if (str[i] == '*' && i + 1 < n && str[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                if (str[i] == '\n') ++line;
                ++i;
            }
            if (!closed) {
                lexErrors.push_back(startLine);
                i = n;
            }
            continue;
        }

        // 标识符/关键字
        if (isIdentStart(c)) {
            int j = i + 1;
            while (j < n && isIdentChar(str[j])) ++j;
            string word = str.substr(i, j - i);
            tokens.emplace_back(getKeywordType(word), word, line);
            i = j;
            continue;
        }

        // 数字常量：严格匹配 0 | [1-9][0-9]*
        if (isDigitChar(c)) {
            int j = i + 1;
            if (c != '0') {
                while (j < n && isDigitChar(str[j])) ++j;
            }
            tokens.emplace_back(TokenType::INT_CONST, str.substr(i, j - i), line);
            i = j;
            continue;
        }

        // 双字符运算符
        if (i + 1 < n) {
            string two = str.substr(i, 2);
            if (two == "==" || two == "!=" || two == ">=" || two == "<=" || two == "&&" || two == "||") {
                tokens.emplace_back(getOpType(two), two, line);
                i += 2;
                continue;
            }
        }

        // 单字符运算符/分隔符
        if (isSingleOp(c)) {
            string one(1, c);
            tokens.emplace_back(getOpType(one), one, line);
            ++i;
            continue;
        }

        // 非法字符（含非 ASCII 字符），跳过并记录行号
        if ((unsigned char)c >= 0x80) {
            int len = utf8Len((unsigned char)c);
            if (i + len > n) len = n - i;
            lexErrors.push_back(line);
            i += len;
            continue;
        }
        lexErrors.push_back(line);
        ++i;
    }
}

// ==========================================================
// 第二部分：文法分析器 (Parser, LL(1) 表驱动)
// ==========================================================

// 将 Token 转换为 LL(1) 表要求的终结符字符串
string getParserTokenStr(const Token& t) {
    if (t.type == TokenType::IDENT) return "ID";
    if (t.type == TokenType::INT_CONST) return "NUMBER";
    return t.lexeme;
}

// 语法分析主函数 (LL(1) 驱动 + 错误恢复)，错误只记录 0-based 行号
void runParser(const vector<Token>& tokens, int lastLine, vector<int>& syntaxErrors) {
    stack<string> st;
    st.push("$");
    st.push(ll1data::kStartSymbol);
    size_t idx = 0;

    while (!st.empty()) {
        string top = st.top();
        string lookahead = (idx < tokens.size()) ? getParserTokenStr(tokens[idx]) : "$";
        int curLine = (idx < tokens.size()) ? tokens[idx].line : lastLine;

        // 如果栈顶是终结符（包含 $）
        if (ll1data::kTerminals.count(top) || top == "$") {
            if (top == lookahead) {
                st.pop();
                idx++;
            } else {
                // 终结符不匹配，报错并弹出栈顶，不消耗输入
                syntaxErrors.push_back(curLine);
                st.pop();
            }
        } else {
            // 栈顶是非终结符
            if (ll1data::kTable.count(top) && ll1data::kTable.at(top).count(lookahead)) {
                int prodId = ll1data::kTable.at(top).at(lookahead);
                st.pop();
                const auto& prod = ll1data::kProductions[prodId - 1];
                for (int i = (int)prod.rhs.size() - 1; i >= 0; --i) {
                    if (!prod.rhs[i].empty()) {
                        st.push(prod.rhs[i]);
                    }
                }
            } else {
                // 非终结符无法匹配当前 lookahead，进行自定义错误恢复
                if (top == "Expr" || top == "LOrExpr" || top == "LAndExpr" || top == "RelExpr" ||
                    top == "AddExpr" || top == "MulExpr" || top == "UnaryExpr" ||
                    top == "PrimaryExpr" || top == "NonIdExpr" || top == "NonIdUnary" ||
                    top == "NonIdPrimary" || top == "ArgOpt" || top == "ArgList") {

                    syntaxErrors.push_back(curLine);
                    st.pop();

                    // 针对 f(, 2) 的特殊恢复：补全 ArgList' 以吞掉逗号
                    if (top == "ArgOpt") {
                        st.push("ArgList'");
                    }
                } else {
                    // 其他非终结符（如 ParamOpt, StmtList'）静默弹出，让错误冒泡到终结符匹配阶段
                    st.pop();
                }
            }
        }
    }
}

// ==========================================================
// 第三部分：抽象语法树 (AST) 与递归下降建树
// ==========================================================

enum class ExprKind { Number, Var, Call, Unary, Binary };

struct Expr {
    ExprKind kind;
    int line;          // 数字 / 标识符 / 运算符所在行
    string name;       // Var / Call 的名称
    string op;         // Unary / Binary 的运算符
    long long num;     // Number 的值
    Expr* a;           // 左操作数 / 一元操作数
    Expr* b;           // 右操作数
    vector<Expr*> args;

    Expr(ExprKind k, int l)
        : kind(k), line(l), num(0), a(nullptr), b(nullptr) {}
};

enum class StmtKind {
    Empty, ExprStmt, Assign, Decl, Block, If, While, Break, Continue, Return
};

struct Stmt {
    StmtKind kind;
    int line;          // 与错误定位相关的行
    string name;       // 声明 / 赋值左侧名称
    bool isConst;      // 是否为 const 声明
    Expr* expr;        // 初始化表达式 / 赋值右值 / 表达式语句 / 条件 / return 值
    Stmt* thenS;
    Stmt* elseS;
    Stmt* body;
    vector<Stmt*> list;
    int endLine;       // Block 的 '}' 所在行
    bool condKnown;    // 条件能否在编译期确定为常量
    long long condValue;

    Stmt(StmtKind k, int l)
        : kind(k), line(l), isConst(false), expr(nullptr), thenS(nullptr),
          elseS(nullptr), body(nullptr), endLine(l), condKnown(false), condValue(0) {}
};

struct ParamDef {
    string name;
    int line;
};

struct FuncAst {
    bool isVoid;
    string name;
    int nameLine;
    vector<ParamDef> params;
    Stmt* body;
    int endLine;
};

struct Program {
    vector<FuncAst> funcs;
    int lastLine;
};

// 在 LL(1) 分析已通过的前提下，把 Token 流组织为带行号的 AST。
class AstBuilder {
public:
    AstBuilder(const vector<Token>& toks, int eofLine)
        : tokens(toks), pos(0), eofLine(eofLine), ok(true) {}

    bool build(Program& prog) {
        while (pos < tokens.size()) {
            FuncAst f;
            if (!parseFuncDef(f)) { ok = false; break; }
            prog.funcs.push_back(f);
        }
        return ok;
    }

    int errorLine() const {
        return pos < tokens.size() ? tokens[pos].line : eofLine;
    }

private:
    const vector<Token>& tokens;
    size_t pos;
    int eofLine;
    bool ok;
    vector<unique_ptr<Expr>> exprPool;
    vector<unique_ptr<Stmt>> stmtPool;

    bool at(TokenType t) const { return pos < tokens.size() && tokens[pos].type == t; }
    bool match(TokenType t) { if (at(t)) { ++pos; return true; } return false; }
    int curLine() const { return pos < tokens.size() ? tokens[pos].line : eofLine; }
    string curLexeme() const { return pos < tokens.size() ? tokens[pos].lexeme : string(); }

    Expr* newExpr(ExprKind k, int line) {
        exprPool.emplace_back(new Expr(k, line));
        return exprPool.back().get();
    }
    Stmt* newStmt(StmtKind k, int line) {
        stmtPool.emplace_back(new Stmt(k, line));
        return stmtPool.back().get();
    }

    static long long parseIntLiteral(const string& s) {
        long long v = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c < '0' || c > '9') break;
            int d = c - '0';
            if (v > (LLONG_MAX - d) / 10) return LLONG_MAX;
            v = v * 10 + d;
        }
        return v;
    }

    bool expect(TokenType t) {
        if (at(t)) { ++pos; return true; }
        ok = false;
        return false;
    }

    bool parseFuncDef(FuncAst& f) {
        f.isVoid = at(TokenType::KW_VOID);
        if (!match(TokenType::KW_INT) && !match(TokenType::KW_VOID)) { ok = false; return false; }

        if (!at(TokenType::IDENT)) { ok = false; return false; }
        f.name = tokens[pos].lexeme;
        f.nameLine = tokens[pos].line;
        ++pos;

        if (!expect(TokenType::LPAREN)) return false;
        if (!at(TokenType::RPAREN)) {
            while (true) {
                if (!expect(TokenType::KW_INT)) return false;
                if (!at(TokenType::IDENT)) { ok = false; return false; }
                ParamDef p;
                p.name = tokens[pos].lexeme;
                p.line = tokens[pos].line;
                ++pos;
                f.params.push_back(p);
                if (match(TokenType::COMMA)) continue;
                break;
            }
        }
        if (!expect(TokenType::RPAREN)) return false;

        f.body = parseBlock();
        if (!f.body) return false;
        f.endLine = f.body->endLine;
        return true;
    }

    Stmt* parseBlock() {
        int startLine = curLine();
        if (!expect(TokenType::LBRACE)) return nullptr;
        Stmt* blk = newStmt(StmtKind::Block, startLine);
        while (!at(TokenType::RBRACE)) {
            if (pos >= tokens.size()) { ok = false; return blk; }
            Stmt* s = parseStmt();
            if (!s) { ok = false; return blk; }
            blk->list.push_back(s);
        }
        blk->endLine = curLine();
        ++pos;  // 消耗 '}'
        return blk;
    }

    Stmt* parseStmt() {
        if (at(TokenType::LBRACE)) return parseBlock();

        if (at(TokenType::SEMICOLON)) {
            int line = curLine();
            ++pos;
            return newStmt(StmtKind::Empty, line);
        }

        if (at(TokenType::KW_INT)) {
            ++pos;
            if (!at(TokenType::IDENT)) { ok = false; return nullptr; }
            int nameLine = curLine();
            string name = curLexeme();
            ++pos;
            Expr* init = nullptr;
            if (match(TokenType::ASSIGN)) {
                init = parseExpr();
                if (!init) return nullptr;
            }
            if (!expect(TokenType::SEMICOLON)) return nullptr;
            Stmt* s = newStmt(StmtKind::Decl, nameLine);
            s->name = name;
            s->isConst = false;
            s->expr = init;
            return s;
        }

        if (at(TokenType::KW_CONST)) {
            ++pos;
            if (!expect(TokenType::KW_INT)) return nullptr;
            if (!at(TokenType::IDENT)) { ok = false; return nullptr; }
            int nameLine = curLine();
            string name = curLexeme();
            ++pos;
            if (!expect(TokenType::ASSIGN)) return nullptr;
            Expr* init = parseExpr();
            if (!init) return nullptr;
            if (!expect(TokenType::SEMICOLON)) return nullptr;
            Stmt* s = newStmt(StmtKind::Decl, nameLine);
            s->name = name;
            s->isConst = true;
            s->expr = init;
            return s;
        }

        if (at(TokenType::KW_IF)) {
            int line = curLine();
            ++pos;
            if (!expect(TokenType::LPAREN)) return nullptr;
            Expr* cond = parseExpr();
            if (!cond) return nullptr;
            if (!expect(TokenType::RPAREN)) return nullptr;
            Stmt* thenS = parseStmt();
            if (!thenS) return nullptr;
            Stmt* elseS = nullptr;
            if (match(TokenType::KW_ELSE)) {
                elseS = parseStmt();
                if (!elseS) return nullptr;
            }
            Stmt* s = newStmt(StmtKind::If, line);
            s->expr = cond;
            s->thenS = thenS;
            s->elseS = elseS;
            return s;
        }

        if (at(TokenType::KW_WHILE)) {
            int line = curLine();
            ++pos;
            if (!expect(TokenType::LPAREN)) return nullptr;
            Expr* cond = parseExpr();
            if (!cond) return nullptr;
            if (!expect(TokenType::RPAREN)) return nullptr;
            Stmt* bodyS = parseStmt();
            if (!bodyS) return nullptr;
            Stmt* s = newStmt(StmtKind::While, line);
            s->expr = cond;
            s->body = bodyS;
            return s;
        }

        if (at(TokenType::KW_BREAK)) {
            int line = curLine();
            ++pos;
            if (!expect(TokenType::SEMICOLON)) return nullptr;
            return newStmt(StmtKind::Break, line);
        }

        if (at(TokenType::KW_CONTINUE)) {
            int line = curLine();
            ++pos;
            if (!expect(TokenType::SEMICOLON)) return nullptr;
            return newStmt(StmtKind::Continue, line);
        }

        if (at(TokenType::KW_RETURN)) {
            int line = curLine();
            ++pos;
            Expr* value = nullptr;
            if (!at(TokenType::SEMICOLON)) {
                value = parseExpr();
                if (!value) return nullptr;
            }
            if (!expect(TokenType::SEMICOLON)) return nullptr;
            Stmt* s = newStmt(StmtKind::Return, line);
            s->expr = value;
            return s;
        }

        // ID '=' Expr ';'
        if (at(TokenType::IDENT) && pos + 1 < tokens.size() &&
            tokens[pos + 1].type == TokenType::ASSIGN) {
            int line = curLine();
            string name = curLexeme();
            pos += 2;
            Expr* rhs = parseExpr();
            if (!rhs) return nullptr;
            if (!expect(TokenType::SEMICOLON)) return nullptr;
            Stmt* s = newStmt(StmtKind::Assign, line);
            s->name = name;
            s->expr = rhs;
            return s;
        }

        // Expr ';'
        Expr* e = parseExpr();
        if (!e) return nullptr;
        if (!expect(TokenType::SEMICOLON)) return nullptr;
        Stmt* s = newStmt(StmtKind::ExprStmt, e->line);
        s->expr = e;
        return s;
    }

    Expr* parseExpr() { return parseLOr(); }

    Expr* parseLOr() {
        Expr* left = parseLAnd();
        if (!left) return nullptr;
        while (at(TokenType::LOGIC_OR)) {
            int line = curLine();
            ++pos;
            Expr* right = parseLAnd();
            if (!right) return nullptr;
            Expr* n = newExpr(ExprKind::Binary, line);
            n->op = "||";
            n->a = left;
            n->b = right;
            left = n;
        }
        return left;
    }

    Expr* parseLAnd() {
        Expr* left = parseRel();
        if (!left) return nullptr;
        while (at(TokenType::LOGIC_AND)) {
            int line = curLine();
            ++pos;
            Expr* right = parseRel();
            if (!right) return nullptr;
            Expr* n = newExpr(ExprKind::Binary, line);
            n->op = "&&";
            n->a = left;
            n->b = right;
            left = n;
        }
        return left;
    }

    Expr* parseRel() {
        Expr* left = parseAdd();
        if (!left) return nullptr;
        while (at(TokenType::LT) || at(TokenType::GT) || at(TokenType::LEQ) ||
               at(TokenType::GEQ) || at(TokenType::EQ) || at(TokenType::NEQ)) {
            int line = curLine();
            string op = curLexeme();
            ++pos;
            Expr* right = parseAdd();
            if (!right) return nullptr;
            Expr* n = newExpr(ExprKind::Binary, line);
            n->op = op;
            n->a = left;
            n->b = right;
            left = n;
        }
        return left;
    }

    Expr* parseAdd() {
        Expr* left = parseMul();
        if (!left) return nullptr;
        while (at(TokenType::PLUS) || at(TokenType::MINUS)) {
            int line = curLine();
            string op = curLexeme();
            ++pos;
            Expr* right = parseMul();
            if (!right) return nullptr;
            Expr* n = newExpr(ExprKind::Binary, line);
            n->op = op;
            n->a = left;
            n->b = right;
            left = n;
        }
        return left;
    }

    Expr* parseMul() {
        Expr* left = parseUnary();
        if (!left) return nullptr;
        while (at(TokenType::STAR) || at(TokenType::SLASH) || at(TokenType::PERCENT)) {
            int line = curLine();
            string op = curLexeme();
            ++pos;
            Expr* right = parseUnary();
            if (!right) return nullptr;
            Expr* n = newExpr(ExprKind::Binary, line);
            n->op = op;
            n->a = left;
            n->b = right;
            left = n;
        }
        return left;
    }

    Expr* parseUnary() {
        if (at(TokenType::PLUS) || at(TokenType::MINUS) || at(TokenType::NOT)) {
            int line = curLine();
            string op = curLexeme();
            ++pos;
            Expr* operand = parseUnary();
            if (!operand) return nullptr;
            Expr* n = newExpr(ExprKind::Unary, line);
            n->op = op;
            n->a = operand;
            return n;
        }
        return parsePrimary();
    }

    Expr* parsePrimary() {
        if (at(TokenType::INT_CONST)) {
            int line = curLine();
            long long v = parseIntLiteral(curLexeme());
            ++pos;
            Expr* n = newExpr(ExprKind::Number, line);
            n->num = v;
            return n;
        }

        if (at(TokenType::IDENT)) {
            int line = curLine();
            string name = curLexeme();
            ++pos;
            if (match(TokenType::LPAREN)) {
                Expr* n = newExpr(ExprKind::Call, line);
                n->name = name;
                if (!at(TokenType::RPAREN)) {
                    while (true) {
                        Expr* arg = parseExpr();
                        if (!arg) return nullptr;
                        n->args.push_back(arg);
                        if (match(TokenType::COMMA)) continue;
                        break;
                    }
                }
                if (!expect(TokenType::RPAREN)) return nullptr;
                return n;
            }
            Expr* n = newExpr(ExprKind::Var, line);
            n->name = name;
            return n;
        }

        if (match(TokenType::LPAREN)) {
            Expr* e = parseExpr();
            if (!e) return nullptr;
            if (!expect(TokenType::RPAREN)) return nullptr;
            return e;
        }

        ok = false;
        return nullptr;
    }
};

// ==========================================================
// 第四部分：语义分析 (Semantic Analyzer)
// ==========================================================

enum class SymKind { Variable, Constant, Parameter, Function };

struct Symbol {
    SymKind kind;
    bool isVoid;          // 函数返回值是否为 void
    int paramCount;       // 函数形参个数
    bool hasConstValue;   // const 变量的值是否可在编译期确定
    long long constValue;

    Symbol()
        : kind(SymKind::Variable), isVoid(false), paramCount(0),
          hasConstValue(false), constValue(0) {}
};

// 表达式的类型信息：isVoid 为 true 时记录该 void 调用的函数名所在行
struct ExprInfo {
    bool isVoid;
    int voidLine;

    ExprInfo(bool v = false, int l = 0) : isVoid(v), voidLine(l) {}
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(vector<int>& errs)
        : errors(errs), loopDepth(0), curFunc(nullptr) {}

    void analyze(const Program& prog) {
        scopes.clear();
        scopes.push_back(map<string, Symbol>());

        // 预置运行时函数
        Symbol getint;
        getint.kind = SymKind::Function;
        getint.isVoid = false;
        getint.paramCount = 0;
        scopes[0]["getint"] = getint;

        Symbol putint;
        putint.kind = SymKind::Function;
        putint.isVoid = true;
        putint.paramCount = 1;
        scopes[0]["putint"] = putint;

        bool mainSeen = false;
        for (size_t i = 0; i < prog.funcs.size(); ++i) {
            const FuncAst& f = prog.funcs[i];

            // 函数按源文件顺序登记；先登记当前函数签名再分析其体，支持自身递归
            if (scopes[0].count(f.name)) {
                error(f.nameLine);  // 函数名全局唯一
            } else {
                Symbol s;
                s.kind = SymKind::Function;
                s.isVoid = f.isVoid;
                s.paramCount = (int)f.params.size();
                scopes[0][f.name] = s;
            }

            if (f.name == "main" && !mainSeen) {
                mainSeen = true;
                if (f.isVoid || !f.params.empty()) error(f.nameLine);
            }

            analyzeFunction(f);
        }

        if (!mainSeen) error(prog.lastLine);
    }

private:
    vector<int>& errors;
    vector<map<string, Symbol>> scopes;
    int loopDepth;
    const FuncAst* curFunc;

    void error(int line) { errors.push_back(line); }

    Symbol* lookup(const string& name) {
        for (int i = (int)scopes.size() - 1; i >= 0; --i) {
            map<string, Symbol>::iterator it = scopes[i].find(name);
            if (it != scopes[i].end()) return &it->second;
        }
        return nullptr;
    }

    // 同一作用域重复声明时保留原绑定，不替换
    void declare(const string& name, const Symbol& sym, int line) {
        map<string, Symbol>& top = scopes.back();
        if (top.count(name)) {
            error(line);
            return;
        }
        top[name] = sym;
    }

    void analyzeFunction(const FuncAst& f) {
        curFunc = &f;
        loopDepth = 0;

        scopes.push_back(map<string, Symbol>());

        for (size_t i = 0; i < f.params.size(); ++i) {
            Symbol s;
            s.kind = SymKind::Parameter;
            declare(f.params[i].name, s, f.params[i].line);
        }

        // 函数形参与函数体最外层局部变量共享同一层作用域
        if (f.body) {
            for (size_t i = 0; i < f.body->list.size(); ++i) {
                analyzeStmt(f.body->list[i]);
            }
        }

        if (!f.isVoid && f.body && !returnsOnAllPaths(f.body)) {
            error(f.body->endLine);
        }

        scopes.pop_back();
        curFunc = nullptr;
    }

    void analyzeStmt(Stmt* s) {
        if (!s) return;

        switch (s->kind) {
        case StmtKind::Empty:
            break;

        case StmtKind::Block: {
            scopes.push_back(map<string, Symbol>());
            for (size_t i = 0; i < s->list.size(); ++i) {
                analyzeStmt(s->list[i]);
            }
            scopes.pop_back();
            break;
        }

        case StmtKind::Decl: {
            if (s->isConst && !s->expr) error(s->line);

            // 初始化表达式在声明点之前分析，因此不能引用变量自身
            if (s->expr) {
                ExprInfo info = analyzeExpr(s->expr);
                if (info.isVoid) error(info.voidLine);
            }

            Symbol sym;
            sym.kind = s->isConst ? SymKind::Constant : SymKind::Variable;
            if (s->isConst && s->expr) {
                long long v = 0;
                if (evalConst(s->expr, v)) {
                    sym.hasConstValue = true;
                    sym.constValue = v;
                }
            }
            declare(s->name, sym, s->line);
            break;
        }

        case StmtKind::Assign: {
            Symbol* sym = lookup(s->name);
            if (!sym) {
                error(s->line);  // 未声明名称
            } else if (sym->kind == SymKind::Function) {
                error(s->line);  // 函数不能作为值存储
            } else if (sym->kind == SymKind::Constant) {
                error(s->line);  // 常量不能作为赋值左值
            }

            ExprInfo info = analyzeExpr(s->expr);
            if (info.isVoid) error(info.voidLine);
            break;
        }

        case StmtKind::ExprStmt:
            if (s->expr) analyzeExpr(s->expr);
            break;

        case StmtKind::If: {
            ExprInfo info = analyzeExpr(s->expr);
            if (info.isVoid) error(info.voidLine);
            s->condKnown = evalConst(s->expr, s->condValue);

            analyzeStmt(s->thenS);
            if (s->elseS) analyzeStmt(s->elseS);
            break;
        }

        case StmtKind::While: {
            ExprInfo info = analyzeExpr(s->expr);
            if (info.isVoid) error(info.voidLine);
            s->condKnown = evalConst(s->expr, s->condValue);

            ++loopDepth;
            analyzeStmt(s->body);
            --loopDepth;
            break;
        }

        case StmtKind::Break:
            if (loopDepth == 0) error(s->line);
            break;

        case StmtKind::Continue:
            if (loopDepth == 0) error(s->line);
            break;

        case StmtKind::Return: {
            if (!curFunc) break;
            if (curFunc->isVoid) {
                if (s->expr) {
                    error(s->line);  // void 函数不能返回值
                    analyzeExpr(s->expr);
                }
            } else {
                if (!s->expr) {
                    error(s->line);  // int 函数必须返回值
                } else {
                    ExprInfo info = analyzeExpr(s->expr);
                    if (info.isVoid) error(info.voidLine);
                }
            }
            break;
        }
        }
    }

    ExprInfo analyzeExpr(Expr* e) {
        if (!e) return ExprInfo();

        switch (e->kind) {
        case ExprKind::Number:
            return ExprInfo();

        case ExprKind::Var: {
            Symbol* sym = lookup(e->name);
            if (!sym) {
                error(e->line);
                return ExprInfo();
            }
            if (sym->kind == SymKind::Function) {
                error(e->line);  // 函数不能作为值使用
                return ExprInfo();
            }
            return ExprInfo();
        }

        case ExprKind::Call: {
            for (size_t i = 0; i < e->args.size(); ++i) {
                ExprInfo ai = analyzeExpr(e->args[i]);
                if (ai.isVoid) error(ai.voidLine);
            }

            Symbol* sym = lookup(e->name);
            if (!sym) {
                error(e->line);
                return ExprInfo();
            }
            if (sym->kind != SymKind::Function) {
                error(e->line);  // 名称种类不符
                return ExprInfo();
            }
            if ((int)e->args.size() != sym->paramCount) {
                error(e->line);  // 实参个数不匹配
            }
            if (sym->isVoid) return ExprInfo(true, e->line);
            return ExprInfo();
        }

        case ExprKind::Unary: {
            ExprInfo ai = analyzeExpr(e->a);
            if (ai.isVoid) error(ai.voidLine);
            return ExprInfo();
        }

        case ExprKind::Binary: {
            ExprInfo li = analyzeExpr(e->a);
            if (li.isVoid) error(li.voidLine);
            ExprInfo ri = analyzeExpr(e->b);
            if (ri.isVoid) error(ri.voidLine);

            if (e->op == "/" || e->op == "%") {
                long long v = 0;
                if (evalConst(e->b, v) && v == 0) error(e->line);
            }
            return ExprInfo();
        }
        }

        return ExprInfo();
    }

    // 编译期常量求值：无法确定时返回 false
    bool evalConst(Expr* e, long long& out) {
        if (!e) return false;

        switch (e->kind) {
        case ExprKind::Number:
            out = e->num;
            return true;

        case ExprKind::Var: {
            Symbol* sym = lookup(e->name);
            if (sym && sym->kind == SymKind::Constant && sym->hasConstValue) {
                out = sym->constValue;
                return true;
            }
            return false;
        }

        case ExprKind::Unary: {
            long long v = 0;
            if (!evalConst(e->a, v)) return false;
            if (e->op == "+") { out = v; return true; }
            if (e->op == "-") { out = -v; return true; }
            if (e->op == "!") { out = (v == 0) ? 1 : 0; return true; }
            return false;
        }

        case ExprKind::Binary: {
            if (e->op == "&&" || e->op == "||") {
                long long l = 0, r = 0;
                bool lok = evalConst(e->a, l);
                bool rok = evalConst(e->b, r);
                if (e->op == "&&") {
                    if (lok && l == 0) { out = 0; return true; }
                    if (rok && r == 0) { out = 0; return true; }
                    if (lok && rok) { out = (l != 0 && r != 0) ? 1 : 0; return true; }
                    return false;
                }
                if (lok && l != 0) { out = 1; return true; }
                if (rok && r != 0) { out = 1; return true; }
                if (lok && rok) { out = (l != 0 || r != 0) ? 1 : 0; return true; }
                return false;
            }

            long long l = 0, r = 0;
            if (!evalConst(e->a, l) || !evalConst(e->b, r)) return false;

            if (e->op == "+") out = l + r;
            else if (e->op == "-") out = l - r;
            else if (e->op == "*") out = l * r;
            else if (e->op == "/") {
                if (r == 0) return false;
                out = l / r;
            } else if (e->op == "%") {
                if (r == 0) return false;
                out = l % r;
            } else if (e->op == "<") out = (l < r) ? 1 : 0;
            else if (e->op == ">") out = (l > r) ? 1 : 0;
            else if (e->op == "<=") out = (l <= r) ? 1 : 0;
            else if (e->op == ">=") out = (l >= r) ? 1 : 0;
            else if (e->op == "==") out = (l == r) ? 1 : 0;
            else if (e->op == "!=") out = (l != r) ? 1 : 0;
            else return false;
            return true;
        }

        case ExprKind::Call:
            return false;
        }

        return false;
    }

    bool returnsOnAllPaths(Stmt* s) {
        if (!s) return false;

        switch (s->kind) {
        case StmtKind::Return:
            return true;

        case StmtKind::Block: {
            for (size_t i = 0; i < s->list.size(); ++i) {
                if (returnsOnAllPaths(s->list[i])) return true;
            }
            return false;
        }

        case StmtKind::If: {
            if (s->condKnown) {
                if (s->condValue != 0) return returnsOnAllPaths(s->thenS);
                return s->elseS ? returnsOnAllPaths(s->elseS) : false;
            }
            if (!s->elseS) return false;
            return returnsOnAllPaths(s->thenS) && returnsOnAllPaths(s->elseS);
        }

        case StmtKind::While: {
            // 条件恒真且循环体内没有跳出当前循环的 break：没有可达出口
            if (s->condKnown && s->condValue != 0 &&
                !containsBreakForCurrentLoop(s->body)) {
                return true;
            }
            return false;
        }

        default:
            return false;
        }
    }

    bool containsBreakForCurrentLoop(Stmt* s) {
        if (!s) return false;

        switch (s->kind) {
        case StmtKind::Break:
            return true;

        case StmtKind::Block: {
            for (size_t i = 0; i < s->list.size(); ++i) {
                if (containsBreakForCurrentLoop(s->list[i])) return true;
            }
            return false;
        }

        case StmtKind::If:
            if (s->condKnown) {
                if (s->condValue != 0) return containsBreakForCurrentLoop(s->thenS);
                return s->elseS ? containsBreakForCurrentLoop(s->elseS) : false;
            }
            return containsBreakForCurrentLoop(s->thenS) ||
                   containsBreakForCurrentLoop(s->elseS);

        case StmtKind::While:
            return false;  // 内层循环的 break 不属于当前循环

        default:
            return false;
        }
    }
};

// ==========================================================
// 第五部分：主函数 (Main)
// ==========================================================

// 文件最后一行的 0-based 行号
int computeLastLine(const string& src) {
    int newlines = 0;
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '\n') ++newlines;
    }
    if (!src.empty() && src[src.size() - 1] == '\n') {
        return newlines > 0 ? newlines - 1 : 0;
    }
    return newlines;
}

// 输出 reject 与按行号从小到大排列、去重后的错误行
void printReject(vector<int> errors) {
    sort(errors.begin(), errors.end());
    errors.erase(unique(errors.begin(), errors.end()), errors.end());

    cout << "reject" << '\n';
    for (size_t i = 0; i < errors.size(); ++i) {
        cout << errors[i] << '\n';
    }
}

int main() {
    string str, line;
    while (getline(cin, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        str += line + "\n";
    }

    // 1. 词法分析
    vector<Token> tokens;
    vector<int> lexErrors;
    runLexer(str, tokens, lexErrors);

    // 2. 语法分析
    vector<int> syntaxErrors;
    int lastTokenLine = tokens.empty() ? 0 : tokens.back().line;
    runParser(tokens, lastTokenLine, syntaxErrors);

    // 存在词法/语法错误时不再进行语义分析
    if (!lexErrors.empty() || !syntaxErrors.empty()) {
        vector<int> allErrors = lexErrors;
        allErrors.insert(allErrors.end(), syntaxErrors.begin(), syntaxErrors.end());
        printReject(allErrors);
        return 0;
    }

    // 3. 构造 AST
    Program prog;
    prog.lastLine = computeLastLine(str);

    AstBuilder builder(tokens, lastTokenLine);
    if (!builder.build(prog)) {
        printReject(vector<int>(1, builder.errorLine()));
        return 0;
    }

    // 4. 语义分析
    vector<int> semErrors;
    SemanticAnalyzer analyzer(semErrors);
    analyzer.analyze(prog);

    if (semErrors.empty()) {
        cout << "accept" << '\n';
    } else {
        printReject(semErrors);
    }

    return 0;
}
