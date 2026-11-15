#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "Lexer.h"

struct Diagnostic {
    int line;
    std::string message;
};

struct Token {
    std::string text;
    int line;
    int column;
};

class Lexer {
public:
    explicit Lexer(const std::string& source) : source_(source) {}

    std::vector<Token> scan(std::vector<Diagnostic>& errors) {
        std::vector<Token> tokens;
        // 词法阶段保留零基行号，供语法错误和语义诊断共用。
        while (!atEnd()) {
            skipWhitespaceAndComments(errors);
            if (atEnd()) {
                break;
            }

            const char ch = peek();
            if (isIdentifierStart(ch)) {
                scanIdentifier(tokens);
            } else if (isDigit(ch)) {
                scanInteger(tokens);
            } else if (!scanSymbol(tokens)) {
                errors.push_back({line_, ""});
                advance();
            }
        }
        tokens.push_back({"<eof>", line_, column_});
        return tokens;
    }

private:
    const std::string& source_;
    std::size_t position_ = 0;
    int line_ = 0;
    int column_ = 1;

    static bool isLetter(char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
    }

    static bool isDigit(char ch) {
        return ch >= '0' && ch <= '9';
    }

    static bool isIdentifierStart(char ch) {
        return isLetter(ch) || ch == '_';
    }

    static bool isIdentifierPart(char ch) {
        return isIdentifierStart(ch) || isDigit(ch);
    }

    bool atEnd() const {
        return position_ >= source_.size();
    }

    char peek(std::size_t offset = 0) const {
        const std::size_t next = position_ + offset;
        return next < source_.size() ? source_[next] : '\0';
    }

    char advance() {
        const char ch = source_[position_++];
        if (ch == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
        return ch;
    }

    void skipWhitespaceAndComments(std::vector<Diagnostic>& errors) {
        while (!atEnd()) {
            if (peek() == ' ' || peek() == '\t' || peek() == '\n' ||
                peek() == '\r' || peek() == '\f' || peek() == '\v') {
                advance();
            } else if (peek() == '/' && peek(1) == '/') {
                while (!atEnd() && peek() != '\n') {
                    advance();
                }
            } else if (peek() == '/' && peek(1) == '*') {
                const int startLine = line_;
                // ToyC 块注释不嵌套，遇到第一个 */ 即结束。
                advance();
                advance();
                while (!atEnd() && !(peek() == '*' && peek(1) == '/')) {
                    advance();
                }
                if (atEnd()) {
                    errors.push_back({startLine, ""});
                    return;
                }
                advance();
                advance();
            } else {
                return;
            }
        }
    }

    void scanIdentifier(std::vector<Token>& tokens) {
        const std::size_t start = position_;
        const int tokenLine = line_;
        const int tokenColumn = column_;
        advance();
        while (isIdentifierPart(peek())) {
            advance();
        }
        tokens.push_back({source_.substr(start, position_ - start),
                          tokenLine, tokenColumn});
    }

    void scanInteger(std::vector<Token>& tokens) {
        const std::size_t start = position_;
        const int tokenLine = line_;
        const int tokenColumn = column_;
        while (isDigit(peek())) {
            advance();
        }
        tokens.push_back({source_.substr(start, position_ - start),
                          tokenLine, tokenColumn});
    }

    bool scanSymbol(std::vector<Token>& tokens) {
        // 先匹配双字符运算符，保证 <=、&& 等不会被拆分。
        static const char* const twoCharacterSymbols[] = {
            "&&", "||", "<=", ">=", "==", "!="
        };
        for (const char* symbol : twoCharacterSymbols) {
            const std::string spelling(symbol);
            if (source_.compare(position_, spelling.size(), spelling) == 0) {
                addSymbol(tokens, spelling);
                advance();
                advance();
                return true;
            }
        }

        static const std::string oneCharacterSymbols = "+-!*/%<>=;,(){}";
        if (oneCharacterSymbols.find(peek()) == std::string::npos) {
            return false;
        }
        const std::string spelling(1, peek());
        addSymbol(tokens, spelling);
        advance();
        return true;
    }

    void addSymbol(std::vector<Token>& tokens, const std::string& spelling) const {
        tokens.push_back({spelling, line_, column_});
    }
};

struct Expr;
struct Stmt;
typedef std::shared_ptr<Expr> ExprPtr;
typedef std::shared_ptr<Stmt> StmtPtr;

struct Expr {
    // 表达式用共享节点表示，left/right 对应一元或二元运算的子节点。
    std::string kind;
    std::string text;
    int line;
    ExprPtr left;
    ExprPtr right;
    std::vector<ExprPtr> arguments;
};

struct Stmt {
    // 语句共用一个节点结构，不适用的字段保持为空。
    std::string kind;
    std::string name;
    std::string declaredType;
    int line;
    bool isConst = false;
    ExprPtr expression;
    ExprPtr initializer;
    StmtPtr firstBranch;
    StmtPtr secondBranch;
    std::vector<StmtPtr> statements;
};

struct Parameter {
    std::string name;
    int line;
};

struct Function {
    std::string returnType;
    std::string name;
    int line;
    std::vector<Parameter> parameters;
    StmtPtr body;
};

struct ParseFailure {
    int line;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens, std::vector<Diagnostic>& errors)
        : tokens_(tokens), errors_(errors) {}

    std::vector<Function> parse() {
        std::vector<Function> functions;
        // 顶层只接受函数定义；出错后跳到下一个函数定义继续分析。
        while (!atEnd()) {
            const std::size_t oldPosition = position_;
            try {
                functions.push_back(parseFunction());
            } catch (const ParseFailure& failure) {
                errors_.push_back({failure.line, ""});
                recoverTopLevel();
            }
            if (position_ == oldPosition && !atEnd()) {
                advance();
            }
        }
        return functions;
    }

private:
    const std::vector<Token>& tokens_;
    std::vector<Diagnostic>& errors_;
    std::size_t position_ = 0;

    const Token& current() const {
        return tokens_[position_ < tokens_.size() ? position_ : tokens_.size() - 1];
    }

    const Token& previous() const {
        return tokens_[position_ == 0 ? 0 : position_ - 1];
    }

    bool atEnd() const {
        return current().text == "<eof>";
    }

    const Token& advance() {
        if (!atEnd()) {
            ++position_;
        }
        return previous();
    }

    bool check(const std::string& text) const {
        return current().text == text;
    }

    bool match(const std::string& text) {
        if (!check(text)) {
            return false;
        }
        advance();
        return true;
    }

    const Token& expect(const std::string& text) {
        if (!check(text)) {
            throw ParseFailure{current().line};
        }
        return advance();
    }

    const Token& expectIdentifier() {
        if (!isIdentifier(current().text)) {
            throw ParseFailure{current().line};
        }
        return advance();
    }

    static bool isIdentifier(const std::string& text) {
        if (text.empty() || text == "<eof>") {
            return false;
        }
        const char first = text[0];
        if (!((first >= 'a' && first <= 'z') ||
              (first >= 'A' && first <= 'Z') || first == '_')) {
            return false;
        }
        for (std::size_t i = 1; i < text.size(); ++i) {
            const char ch = text[i];
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                  (ch >= '0' && ch <= '9') || ch == '_')) {
                return false;
            }
        }
        static const std::unordered_set<std::string> keywords = {
            "int", "void", "const", "if", "else", "while",
            "break", "continue", "return"
        };
        return keywords.count(text) == 0;
    }

    bool isType() const {
        return check("int") || check("void");
    }

    Function parseFunction() {
        Function function;
        if (!isType()) {
            throw ParseFailure{current().line};
        }
        function.returnType = advance().text;
        const Token& name = expectIdentifier();
        function.name = name.text;
        function.line = name.line;
        expect("(");

        if (!check(")")) {
            do {
                expect("int");
                const Token& parameterName = expectIdentifier();
                function.parameters.push_back({parameterName.text, parameterName.line});
            } while (match(","));
        }

        expect(")");
        function.body = parseBlock();
        return function;
    }

    StmtPtr parseBlock() {
        const Token& opening = expect("{");
        StmtPtr block = std::make_shared<Stmt>();
        block->kind = "Block";
        block->line = opening.line;

        // 块内错误在分号或右花括号处同步，尽量继续找后续错误。
        while (!atEnd() && !check("}")) {
            const std::size_t oldPosition = position_;
            try {
                block->statements.push_back(parseStatement());
            } catch (const ParseFailure& failure) {
                errors_.push_back({failure.line, ""});
                recoverStatement();
            }
            if (position_ == oldPosition && !atEnd() && !check("}")) {
                advance();
            }
        }

        expect("}");
        return block;
    }

    StmtPtr parseStatement() {
        if (check("{")) {
            return parseBlock();
        }
        if (match(";")) {
            return makeStatement("Empty", previous().line);
        }
        if (match("const")) {
            const Token& start = previous();
            expect("int");
            const Token& name = expectIdentifier();
            expect("=");
            StmtPtr declaration = makeStatement("Declaration", start.line);
            declaration->name = name.text;
            declaration->declaredType = "int";
            declaration->isConst = true;
            declaration->initializer = parseExpression();
            expect(";");
            return declaration;
        }
        if (match("int")) {
            const Token& start = previous();
            const Token& name = expectIdentifier();
            StmtPtr declaration = makeStatement("Declaration", start.line);
            declaration->name = name.text;
            declaration->declaredType = "int";
            if (match("=")) {
                declaration->initializer = parseExpression();
            }
            expect(";");
            return declaration;
        }
        if (match("if")) {
            const Token& start = previous();
            expect("(");
            ExprPtr condition = parseExpression();
            expect(")");
            StmtPtr statement = makeStatement("If", start.line);
            statement->expression = condition;
            statement->firstBranch = parseStatement();
            if (match("else")) {
                statement->secondBranch = parseStatement();
            }
            return statement;
        }
        if (match("while")) {
            const Token& start = previous();
            if (!match("(")) {
                // while 缺少左括号时，右括号也无法与条件匹配，分别报告两处。
                errors_.push_back({current().line, ""});
                throw ParseFailure{current().line};
            }
            StmtPtr statement = makeStatement("While", start.line);
            statement->expression = parseExpression();
            expect(")");
            statement->firstBranch = parseStatement();
            return statement;
        }
        if (match("break")) {
            const Token& start = previous();
            expect(";");
            return makeStatement("Break", start.line);
        }
        if (match("continue")) {
            const Token& start = previous();
            expect(";");
            return makeStatement("Continue", start.line);
        }
        if (match("return")) {
            const Token& start = previous();
            StmtPtr statement = makeStatement("Return", start.line);
            if (!check(";")) {
                statement->expression = parseExpression();
            }
            expect(";");
            return statement;
        }

        const int line = current().line;
        ExprPtr expression = parseExpression();
        if (expression->kind == "Variable" && match("=")) {
            StmtPtr assignment = makeStatement("Assignment", line);
            assignment->name = expression->text;
            assignment->expression = parseExpression();
            expect(";");
            return assignment;
        }

        StmtPtr expressionStatement = makeStatement("Expression", line);
        expressionStatement->expression = expression;
        expect(";");
        return expressionStatement;
    }

    static StmtPtr makeStatement(const std::string& kind, int line) {
        StmtPtr statement = std::make_shared<Stmt>();
        statement->kind = kind;
        statement->line = line;
        return statement;
    }

    ExprPtr parseExpression() {
        return parseLogicalOr();
    }

    ExprPtr parseLogicalOr() {
        // 按 C 的优先级分层解析；循环结构使二元运算保持左结合。
        ExprPtr expression = parseLogicalAnd();
        while (match("||")) {
            expression = makeBinary("||", expression, parseLogicalAnd());
        }
        return expression;
    }

    ExprPtr parseLogicalAnd() {
        ExprPtr expression = parseRelational();
        while (match("&&")) {
            expression = makeBinary("&&", expression, parseRelational());
        }
        return expression;
    }

    ExprPtr parseRelational() {
        ExprPtr expression = parseAdditive();
        while (check("<") || check(">") || check("<=") || check(">=") ||
               check("==") || check("!=")) {
            const std::string op = advance().text;
            expression = makeBinary(op, expression, parseAdditive());
        }
        return expression;
    }

    ExprPtr parseAdditive() {
        ExprPtr expression = parseMultiplicative();
        while (check("+") || check("-")) {
            const std::string op = advance().text;
            expression = makeBinary(op, expression, parseMultiplicative());
        }
        return expression;
    }

    ExprPtr parseMultiplicative() {
        ExprPtr expression = parseUnary();
        while (check("*") || check("/") || check("%")) {
            const std::string op = advance().text;
            expression = makeBinary(op, expression, parseUnary());
        }
        return expression;
    }

    ExprPtr parseUnary() {
        if (check("+") || check("-") || check("!")) {
            const Token& op = advance();
            ExprPtr expression = std::make_shared<Expr>();
            expression->kind = "Unary";
            expression->text = op.text;
            expression->line = op.line;
            expression->left = parseUnary();
            return expression;
        }
        return parsePrimary();
    }

    ExprPtr parsePrimary() {
        if (match("(")) {
            ExprPtr expression = parseExpression();
            expect(")");
            return expression;
        }

        if (isInteger(current().text)) {
            const Token& number = advance();
            ExprPtr expression = std::make_shared<Expr>();
            expression->kind = "Number";
            expression->text = number.text;
            expression->line = number.line;
            return expression;
        }

        if (isIdentifier(current().text)) {
            const Token& identifier = advance();
            ExprPtr expression = std::make_shared<Expr>();
            expression->kind = "Variable";
            expression->text = identifier.text;
            expression->line = identifier.line;
            if (match("(")) {
                expression->kind = "Call";
                if (!check(")")) {
                    do {
                        expression->arguments.push_back(parseExpression());
                    } while (match(","));
                }
                expect(")");
            }
            return expression;
        }

        throw ParseFailure{current().line};
    }

    static bool isInteger(const std::string& text) {
        if (text.empty()) {
            return false;
        }
        for (char ch : text) {
            if (ch < '0' || ch > '9') {
                return false;
            }
        }
        return true;
    }

    static ExprPtr makeBinary(const std::string& op,
                              const ExprPtr& left,
                              const ExprPtr& right) {
        ExprPtr expression = std::make_shared<Expr>();
        expression->kind = "Binary";
        expression->text = op;
        expression->line = left->line;
        expression->left = left;
        expression->right = right;
        return expression;
    }

    void recoverStatement() {
        int nestedBlocks = 0;
        while (!atEnd()) {
            if (check("{")) {
                ++nestedBlocks;
                advance();
            } else if (check("}")) {
                if (nestedBlocks == 0) {
                    return;
                }
                --nestedBlocks;
                advance();
                if (nestedBlocks == 0) {
                    return;
                }
            } else if (check(";") && nestedBlocks == 0) {
                advance();
                return;
            } else {
                advance();
            }
        }
    }

    void recoverTopLevel() {
        while (!atEnd() && !check("int") && !check("void")) {
            advance();
        }
    }
};

struct FunctionInfo {
    std::string returnType;
    std::size_t parameterCount;
    int line;
};

struct VariableInfo {
    bool isConst;
    int line;
};

enum ValueType {
    IntegerValue,
    VoidValue,
    InvalidValue
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(std::vector<Diagnostic>& errors) : errors_(errors) {}

    void analyze(const std::vector<Function>& functions) {
        bool foundMain = false;
        // ToyC 运行库函数在用户源码之外提供，默认视为已声明。
        visibleFunctions_["getint"] = {"int", 0, 0};
        visibleFunctions_["putint"] = {"void", 1, 0};
        visibleFunctions_["putch"] = {"void", 1, 0};

        for (const Function& function : functions) {
            // 每个函数在检查函数体前登记自身，因此允许递归，但不允许前向调用。
            if (visibleFunctions_.count(function.name) != 0) {
                addError(function.line, "redefinition of function '" + function.name + "'");
                continue;
            }

            visibleFunctions_[function.name] = {
                function.returnType, function.parameters.size(), function.line
            };
            if (function.name == "main") {
                foundMain = true;
                if (function.returnType != "int" || !function.parameters.empty()) {
                    addError(function.line, "'main' must have type 'int()'");
                }
            }

            scopes_.clear();
            scopes_.push_back(std::unordered_map<std::string, VariableInfo>());
            currentFunction_ = &function;
            loopDepth_ = 0;

            for (const Parameter& parameter : function.parameters) {
                declareVariable(parameter.name, false, parameter.line);
            }

            analyzeBlockContents(function.body, false);
            if (function.returnType == "int" && !definitelyReturns(function.body)) {
                addError(function.line, "control reaches end of non-void function '" +
                         function.name + "'");
            }
        }

        if (!foundMain) {
            addError(0, "program must define 'int main()'");
        }
    }

private:
    std::vector<Diagnostic>& errors_;
    std::unordered_map<std::string, FunctionInfo> visibleFunctions_;
    std::vector<std::unordered_map<std::string, VariableInfo> > scopes_;
    const Function* currentFunction_ = NULL;
    int loopDepth_ = 0;

    void addError(int line, const std::string& message) {
        errors_.push_back({line, message});
    }

    void declareVariable(const std::string& name, bool isConst, int line) {
        std::unordered_map<std::string, VariableInfo>& scope = scopes_.back();
        if (scope.count(name) != 0) {
            addError(line, "redefinition of '" + name + "'");
            return;
        }
        scope[name] = {isConst, line};
    }

    const VariableInfo* findVariable(const std::string& name) const {
        // 从最内层作用域向外查找，实现局部变量屏蔽外层同名变量。
        for (std::vector<std::unordered_map<std::string, VariableInfo> >::const_reverse_iterator
                 scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            std::unordered_map<std::string, VariableInfo>::const_iterator found =
                scope->find(name);
            if (found != scope->end()) {
                return &found->second;
            }
        }
        return NULL;
    }

    void analyzeBlockContents(const StmtPtr& block, bool createScope) {
        if (!block || block->kind != "Block") {
            return;
        }
        if (createScope) {
            scopes_.push_back(std::unordered_map<std::string, VariableInfo>());
        }
        for (const StmtPtr& statement : block->statements) {
            analyzeStatement(statement);
        }
        if (createScope) {
            scopes_.pop_back();
        }
    }

    void analyzeStatement(const StmtPtr& statement) {
        if (!statement) {
            return;
        }
        if (statement->kind == "Block") {
            analyzeBlockContents(statement, true);
        } else if (statement->kind == "Declaration") {
            if (!statement->initializer) {
                addError(statement->line, "variable '" + statement->name +
                         "' must be initialized");
            } else {
                requireInteger(analyzeExpression(statement->initializer),
                               statement->initializer->line);
            }
            declareVariable(statement->name, statement->isConst, statement->line);
        } else if (statement->kind == "Assignment") {
            const VariableInfo* variable = findVariable(statement->name);
            if (!variable) {
                addUndeclared(statement->name, statement->line);
            } else if (variable->isConst) {
                addError(statement->line, "assignment of read-only variable '" +
                         statement->name + "'");
            }
            requireInteger(analyzeExpression(statement->expression),
                           statement->expression->line);
        } else if (statement->kind == "Expression") {
            analyzeExpression(statement->expression);
        } else if (statement->kind == "If") {
            requireInteger(analyzeExpression(statement->expression),
                           statement->expression->line);
            analyzeStatement(statement->firstBranch);
            analyzeStatement(statement->secondBranch);
        } else if (statement->kind == "While") {
            requireInteger(analyzeExpression(statement->expression),
                           statement->expression->line);
            ++loopDepth_;
            analyzeStatement(statement->firstBranch);
            --loopDepth_;
        } else if (statement->kind == "Break" || statement->kind == "Continue") {
            if (loopDepth_ == 0) {
                addError(statement->line,
                         statement->kind == "Break"
                             ? "break statement not within loop or switch"
                             : "continue statement not within a loop");
            }
        } else if (statement->kind == "Return") {
            if (currentFunction_->returnType == "void") {
                if (statement->expression) {
                    analyzeExpression(statement->expression);
                    addError(statement->line,
                             "return with a value in function returning void");
                }
            } else if (!statement->expression) {
                addError(statement->line,
                         "return with no value in function returning non-void");
            } else {
                requireInteger(analyzeExpression(statement->expression),
                               statement->expression->line);
            }
        }
    }

    ValueType analyzeExpression(const ExprPtr& expression) {
        // 表达式检查同时返回类型，用来发现 void 调用被当作整数值使用。
        if (!expression) {
            return InvalidValue;
        }
        if (expression->kind == "Number") {
            return IntegerValue;
        }
        if (expression->kind == "Variable") {
            if (findVariable(expression->text)) {
                return IntegerValue;
            }
            if (visibleFunctions_.count(expression->text) != 0) {
                addError(expression->line, "function '" + expression->text +
                         "' used as a value");
            } else {
                addUndeclared(expression->text, expression->line);
            }
            return InvalidValue;
        }
        if (expression->kind == "Call") {
            for (const ExprPtr& argument : expression->arguments) {
                requireInteger(analyzeExpression(argument), argument->line);
            }
            std::unordered_map<std::string, FunctionInfo>::const_iterator function =
                visibleFunctions_.find(expression->text);
            if (function == visibleFunctions_.end()) {
                addError(expression->line, "call to undeclared function '" +
                         expression->text + "'");
                return InvalidValue;
            }
            if (expression->arguments.size() < function->second.parameterCount) {
                addError(expression->line, "too few arguments to function '" +
                         expression->text + "'");
            } else if (expression->arguments.size() > function->second.parameterCount) {
                addError(expression->line, "too many arguments to function '" +
                         expression->text + "'");
            }
            return function->second.returnType == "void" ? VoidValue : IntegerValue;
        }
        if (expression->kind == "Unary") {
            requireInteger(analyzeExpression(expression->left), expression->line);
            return IntegerValue;
        }

        const ValueType leftType = analyzeExpression(expression->left);
        const ValueType rightType = analyzeExpression(expression->right);
        requireInteger(leftType, expression->left->line);
        requireInteger(rightType, expression->right->line);
        return leftType == InvalidValue || rightType == InvalidValue
                   ? InvalidValue : IntegerValue;
    }

    void requireInteger(ValueType type, int line) {
        if (type == VoidValue) {
            addError(line, "void value not ignored as it ought to be");
        }
    }

    void addUndeclared(const std::string& name, int line) {
        addError(line, "'" + name + "' undeclared (first use in this function)");
    }

    static bool definitelyReturns(const StmtPtr& statement) {
        // 仅当顺序语句已返回，或 if 的两个分支都返回时认定路径完整。
        if (!statement) {
            return false;
        }
        if (statement->kind == "Return") {
            return true;
        }
        if (statement->kind == "Block") {
            for (const StmtPtr& child : statement->statements) {
                if (definitelyReturns(child)) {
                    return true;
                }
            }
            return false;
        }
        if (statement->kind == "If") {
            return statement->secondBranch &&
                   definitelyReturns(statement->firstBranch) &&
                   definitelyReturns(statement->secondBranch);
        }
        return false;
    }
};

static void printExpression(std::ostream& out, const ExprPtr& expression, int depth) {
    if (!expression) {
        return;
    }
    const std::string indent(static_cast<std::size_t>(depth) * 2, ' ');
    if (expression->kind == "Binary") {
        out << indent << "BinaryExpr " << expression->text << '\n';
        printExpression(out, expression->left, depth + 1);
        printExpression(out, expression->right, depth + 1);
    } else if (expression->kind == "Unary") {
        out << indent << "UnaryExpr " << expression->text << '\n';
        printExpression(out, expression->left, depth + 1);
    } else if (expression->kind == "Call") {
        out << indent << "Call " << expression->text << '\n';
        for (const ExprPtr& argument : expression->arguments) {
            printExpression(out, argument, depth + 1);
        }
    } else {
        out << indent << expression->kind << ' ' << expression->text
            << " [行 " << expression->line << "]\n";
    }
}

static void printStatement(std::ostream& out, const StmtPtr& statement, int depth) {
    if (!statement) {
        return;
    }
    const std::string indent(static_cast<std::size_t>(depth) * 2, ' ');
    if (statement->kind == "Block") {
        out << indent << "Block\n";
        for (const StmtPtr& child : statement->statements) {
            printStatement(out, child, depth + 1);
        }
    } else if (statement->kind == "Declaration") {
        out << indent << (statement->isConst ? "ConstDecl " : "VarDecl ")
            << statement->name << " [行 " << statement->line << "]\n";
        printExpression(out, statement->initializer, depth + 1);
    } else if (statement->kind == "Assignment") {
        out << indent << "Assign " << statement->name << '\n';
        printExpression(out, statement->expression, depth + 1);
    } else if (statement->kind == "Expression") {
        out << indent << "ExprStmt\n";
        printExpression(out, statement->expression, depth + 1);
    } else if (statement->kind == "If") {
        out << indent << "If\n" << indent << "  Condition\n";
        printExpression(out, statement->expression, depth + 2);
        out << indent << "  Then\n";
        printStatement(out, statement->firstBranch, depth + 2);
        if (statement->secondBranch) {
            out << indent << "  Else\n";
            printStatement(out, statement->secondBranch, depth + 2);
        }
    } else if (statement->kind == "While") {
        out << indent << "While\n" << indent << "  Condition\n";
        printExpression(out, statement->expression, depth + 2);
        printStatement(out, statement->firstBranch, depth + 1);
    } else if (statement->kind == "Return") {
        out << indent << "Return\n";
        printExpression(out, statement->expression, depth + 1);
    } else {
        out << indent << statement->kind << " [行 " << statement->line << "]\n";
    }
}

static bool writeTree(const std::vector<Function>& functions) {
    // 树文件用缩进表达父子关系，并附上函数、参数和表达式的词素信息。
    std::ofstream tree("tree.txt");
    if (!tree) {
        return false;
    }

    tree << "CompUnit\n";
    for (const Function& function : functions) {
        tree << "  FuncDef " << function.returnType << ' ' << function.name
             << " [行 " << function.line << "]\n";
        tree << "    Parameters\n";
        for (const Parameter& parameter : function.parameters) {
            tree << "      int " << parameter.name << " [行 "
                 << parameter.line << "]\n";
        }
        printStatement(tree, function.body, 2);
    }
    return true;
}

static void printResult(const std::vector<Diagnostic>& errors) {
    if (errors.empty()) {
        std::cout << "accept\n";
        return;
    }

    std::cout << "reject\n";
    for (const Diagnostic& error : errors) {
        std::cout << error.line;
        if (!error.message.empty()) {
            std::cout << ' ' << error.message;
        }
        std::cout << '\n';
    }
}

static bool isNumberToken(const std::string& text) {
    return !text.empty() && text[0] >= '0' && text[0] <= '9';
}

static void printTokenResult(const std::vector<toy::Token>& tokens,
                             const std::vector<toy::LexError>& errors) {
    if (!errors.empty()) {
        std::cout << "reject\n";
        for (const toy::LexError& error : errors) {
            std::cout << error.line << ' ' << error.message << '\n';
        }
        std::cout << '\n';
    }
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const toy::Token& token = tokens[i];
        std::string type = token.type;
        if (type.empty()) {
            type = isNumberToken(token.text) ? "IntConst" : "Ident";
        }
        std::cout << i << ':' << type << ":\"" << token.text << "\"\n";
    }
}

int main(int argc, char** argv) {
    std::string inputPath;
    bool tokenMode = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-token" || arg == "--token") {
            tokenMode = true;
            continue;
        }
        if (arg == "-ast" || arg == "--ast") {
            continue;
        }
        if (arg == "-opt" || arg == "--opt") {
            continue;
        }
        if (arg == "-asm" || arg == "--asm") {
            std::cerr << "assembly generation is not implemented in this stage\n";
            return 2;
        }
        if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                ++i;
            }
            continue;
        }
        if (!arg.empty() && arg[0] != '-' && inputPath.empty()) {
            inputPath = arg;
        }
        // 评测驱动可能附带其它兼容参数；本阶段只需要 -ast，忽略其余选项。
    }
    std::string source;
    if (inputPath.empty()) {
        source.assign(std::istreambuf_iterator<char>(std::cin),
                      std::istreambuf_iterator<char>());
    } else {
        std::ifstream input(inputPath.c_str(), std::ios::binary);
        if (!input) {
            std::cerr << "unable to open input: " << inputPath << '\n';
            return 2;
        }
        source.assign(std::istreambuf_iterator<char>(input),
                      std::istreambuf_iterator<char>());
    }

    if (tokenMode) {
        toy::Lexer lexer(source);
        std::vector<toy::Token> tokens;
        std::vector<toy::LexError> errors;
        lexer.scan(tokens, errors);
        printTokenResult(tokens, errors);
        return 0;
    }

    std::vector<Diagnostic> errors;
    Lexer lexer(source);
    const std::vector<Token> tokens = lexer.scan(errors);
    Parser parser(tokens, errors);
    const std::vector<Function> functions = parser.parse();

    if (errors.empty()) {
        SemanticAnalyzer analyzer(errors);
        analyzer.analyze(functions);
    }

    if (!writeTree(functions)) {
        std::cerr << "Unable to write tree.txt\n";
        return 1;
    }
    printResult(errors);
    return errors.empty() ? 0 : 1;
}
