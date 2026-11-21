#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "tree.h"
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

struct Expr;
struct Stmt;
typedef std::shared_ptr<Expr> ExprPtr;
typedef std::shared_ptr<Stmt> StmtPtr;

struct Expr {
    std::string kind;
    std::string text;
    int line;
    ExprPtr left;
    ExprPtr right;
    std::vector<ExprPtr> arguments;
};

struct Stmt {
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
        // Parse top-level function definitions.
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

        // Parse statements until the closing brace.
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
                // A break or continue outside a loop is a syntax error here.
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
        // Parse logical-or expressions from the next precedence level.
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
            if (statement->initializer) {
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
            << " [??" << expression->line << "]\n";
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
            << statement->name << " [??" << statement->line << "]\n";
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
        out << indent << statement->kind << " [??" << statement->line << "]\n";
    }
}

static bool writeTree(const std::vector<Function>& functions) {
    std::ofstream tree("tree.txt");
    if (!tree) {
        return false;
    }

    tree << "CompUnit\n";
    for (const Function& function : functions) {
        tree << "  FuncDef " << function.returnType << ' ' << function.name
             << " [??" << function.line << "]\n";
        tree << "    Parameters\n";
        for (const Parameter& parameter : function.parameters) {
            tree << "      int " << parameter.name << " [??"
                 << parameter.line << "]\n";
        }
        printStatement(tree, function.body, 2);
    }
    return true;
}

static ASTNodePtr buildASTExpression(const ExprPtr& expression) {
    if (!expression) return ASTNodePtr();
    ASTNodePtr node;
    if (expression->kind == "Binary") {
        node = makeASTNode("Binary");
        addASTAttribute(node, "op", expression->text);
        node->children.push_back(buildASTExpression(expression->left));
        node->children.push_back(buildASTExpression(expression->right));
    } else if (expression->kind == "Unary") {
        node = makeASTNode("Unary");
        addASTAttribute(node, "op", expression->text);
        node->children.push_back(buildASTExpression(expression->left));
    } else if (expression->kind == "Call") {
        node = makeASTNode("Call");
        addASTAttribute(node, "name", expression->text);
        for (const ExprPtr& argument : expression->arguments) {
            node->children.push_back(buildASTExpression(argument));
        }
    } else if (expression->kind == "Number") {
        node = makeASTNode("Number");
        addASTAttribute(node, "value", expression->text);
    } else {
        node = makeASTNode("Variable");
        addASTAttribute(node, "name", expression->text);
    }
    return node;
}

static ASTNodePtr buildASTStatement(const StmtPtr& statement) {
    if (!statement) return ASTNodePtr();
    ASTNodePtr node;
    if (statement->kind == "Block") {
        node = makeASTNode("Block");
        for (const StmtPtr& child : statement->statements) {
            node->children.push_back(buildASTStatement(child));
        }
    } else if (statement->kind == "Declaration") {
        node = makeASTNode("Decl");
        addASTAttribute(node, "name", statement->name);
        addASTAttribute(node, "is_const", statement->isConst ? "true" : "false");
        node->children.push_back(buildASTExpression(statement->initializer));
    } else if (statement->kind == "Assignment") {
        node = makeASTNode("Assign");
        addASTAttribute(node, "name", statement->name);
        node->children.push_back(buildASTExpression(statement->expression));
    } else if (statement->kind == "Expression") {
        node = buildASTExpression(statement->expression);
    } else if (statement->kind == "If") {
        node = makeASTNode("If");
        node->children.push_back(buildASTExpression(statement->expression));
        node->children.push_back(buildASTStatement(statement->firstBranch));
        node->children.push_back(buildASTStatement(statement->secondBranch));
    } else if (statement->kind == "While") {
        node = makeASTNode("While");
        node->children.push_back(buildASTExpression(statement->expression));
        node->children.push_back(buildASTStatement(statement->firstBranch));
    } else if (statement->kind == "Return") {
        node = makeASTNode("Return");
        node->children.push_back(buildASTExpression(statement->expression));
    } else if (statement->kind == "Break" || statement->kind == "Continue" ||
               statement->kind == "Empty") {
        node = makeASTNode(statement->kind);
    } else {
        node = makeASTNode(statement->kind);
    }
    return node;
}

static ASTNodePtr buildAST(const std::vector<Function>& functions) {
    ASTNodePtr program = makeASTNode("Program");
    for (const Function& function : functions) {
        ASTNodePtr fn = makeASTNode("Function");
        addASTAttribute(fn, "return_type", function.returnType);
        addASTAttribute(fn, "name", function.name);
        std::string params;
        for (std::size_t i = 0; i < function.parameters.size(); ++i) {
            if (i != 0) {
                params += ",";
            }
            params += function.parameters[i].name;
        }
        addASTAttribute(fn, "params", params);
        fn->children.push_back(buildASTStatement(function.body));
        program->children.push_back(fn);
    }
    return program;
}

static void printResult(const std::vector<Diagnostic>& errors) {
    if (errors.empty()) {
        std::cout << "accept\n";
        return;
    }

    std::cout << "reject\n";
    std::set<int> reportedLines;
    for (const Diagnostic& error : errors) {
        reportedLines.insert(error.line);
    }
    for (int line : reportedLines) {
        std::cout << line << '\n';
    }
}

int main() {
    const std::string source((std::istreambuf_iterator<char>(std::cin)),
                             std::istreambuf_iterator<char>());
    std::vector<Diagnostic> errors;
    toy::Lexer lexer(source);
    std::vector<toy::Token> scannedTokens;
    std::vector<toy::LexError> lexErrors;
    lexer.scan(scannedTokens, lexErrors);
    std::vector<Token> tokens;
    for (const toy::Token& token : scannedTokens) {
        tokens.push_back({token.text, token.line, token.column});
    }
    for (const toy::LexError& error : lexErrors) {
        errors.push_back({error.line, ""});
    }
    // 第一部分的 Lexer 不添加结束标记；解析器使用源码末尾的位置。
    int eofLine = 0;
    int eofColumn = 1;
    for (char ch : source) {
        if (ch == '\n') {
            ++eofLine;
            eofColumn = 1;
        } else {
            ++eofColumn;
        }
    }
    tokens.push_back({"<eof>", eofLine, eofColumn});
    Parser parser(tokens, errors);
    const std::vector<Function> functions = parser.parse();

    if (!writeTree(functions)) {
        std::cerr << "Unable to write tree.txt\n";
        return 1;
    }
    if (errors.empty()) {
        SemanticAnalyzer analyzer(errors);
        analyzer.analyze(functions);
    }
    (void)buildAST; // AST file output is temporarily disabled.
    printResult(errors);
    return 0;
}

