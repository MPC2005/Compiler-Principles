// 独立的表驱动 LL(1) 语法分析程序。
//
// 分析表在编译期由 LL1TableData.h 提供，运行时不再读取 JSON：
//   kTable[非终结符][向前看终结符] = 产生式 id；没有的格子就是 error
//   kProductions[id-1] 给出 lhs、rhs 和可读文本；rhs 为空表示 ε
//   kTerminals 是字面终结符集合（$ 由程序单独处理）
//
// 数据源仍是 LL(1)分析表_BNF文法.json；
// 文法或分析表改动后运行：python tools/gen_table_header.py
//
// 输出：
//   无语法错误：一行 accept
//   有错误：一行 reject，随后每个错误一行行号（0 基）
// 同时生成 tree.txt，展示语法分析树。
//
// 用法：
//   compiler -ast
// 源码从标准输入读取；-v 把查表/规约过程打到 stderr。

#include "LL1TableData.h"
#include "Lexer.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <set>
#include <vector>

// ---------------------------------------------------------------------------
// 直接使用编译期生成的嵌套字典：
//   kTable[非终结符][向前看终结符] = 产生式 id
// 字典里没有的格子就是 error。
// ---------------------------------------------------------------------------
int lookup(const std::string& nonterminal, const std::string& terminal) {
    std::map<std::string, std::map<std::string, int> >::const_iterator row =
        ll1data::kTable.find(nonterminal);
    if (row == ll1data::kTable.end()) {
        return -1;
    }
    std::map<std::string, int>::const_iterator cell =
        row->second.find(terminal);
    if (cell == row->second.end()) {
        return -1;
    }
    return cell->second;
}

// ---------------------------------------------------------------------------
// 词法与 token。
// ---------------------------------------------------------------------------
// 词法单元：原文 + 0 基行号 + 1 基列号。
// 关键字、标识符、数字、符号暂时不分类，交给 terminalOf 归一成分析表的终结符。
struct Token {
    std::string text;
    int line;
    int column;
};

// 错误记录：按 错误处理.txt 的 report_error(expect, actual, line, col) 保存四元组。
// stdout 只输出 line；expect/actual/column 在 -v 时打印到 stderr。
struct Error {
    int line;
    int column;
    std::string expected;
    std::string actual;
};

std::vector<Token> lexShared(const std::string& source, std::vector<Error>& errors) {
    toy::Lexer lexer(source);
    std::vector<toy::Token> sharedTokens;
    std::vector<toy::LexError> lexErrors;
    lexer.scan(sharedTokens, lexErrors);
    std::vector<Token> tokens;
    for (const toy::Token& token : sharedTokens) {
        tokens.push_back({token.text, token.line, token.column});
    }
    for (const toy::LexError& error : lexErrors) {
        errors.push_back({error.line, error.column, "", ""});
    }
    Token eof;
    eof.text = "<eof>";
    eof.line = sharedTokens.empty() ? 0 : sharedTokens.back().line;
    eof.column = sharedTokens.empty() ? 1 : sharedTokens.back().column;
    tokens.push_back(eof);
    return tokens;
}

// 把词法 token 归一到分析表里的终结符名：
//   <eof>      -> $
//   关键字/符号 -> 原文本身（int、if、(、== 等）
//   纯数字      -> NUMBER
//   其他        -> ID
std::string terminalOf(const Token& token) {
    if (token.text == "<eof>") {
        return "$";
    }
    if (ll1data::kTerminals.count(token.text) != 0) {
        return token.text;
    }
    bool allDigits = !token.text.empty();
    for (std::size_t i = 0; i < token.text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(token.text[i]))) {
            allDigits = false;
            break;
        }
    }
    return allDigits ? "NUMBER" : "ID";
}

// ---------------------------------------------------------------------------
// 语法树节点。
// label 是打印出来的名字，children 按源码中的先后顺序排列。
// ---------------------------------------------------------------------------
struct Node {
    std::string label;
    std::vector<std::shared_ptr<Node> > children;
};

typedef std::shared_ptr<Node> NodePtr;

// 建一个普通节点。
NodePtr makeNode(const std::string& label) {
    NodePtr node = std::make_shared<Node>();
    node->label = label;
    return node;
}

// 终结符叶子：ID/NUMBER 附带具体词素，其它终结符直接用拼写。
NodePtr makeLeafNode(const std::string& terminal, const std::string& text) {
    if (terminal == "ID" || terminal == "NUMBER") {
        return makeNode(terminal + "(" + text + ")");
    }
    return makeNode(terminal);
}

// 错误恢复时插入的占位节点，方便在 tree.txt 里看出哪条语句被丢弃。
NodePtr makeErrorNode(int line) {
    return makeNode("ERROR line " + std::to_string(line));
}

// 递归打印树，每深一层缩进两个空格。
void printNode(std::ostream& out, const NodePtr& node, int depth) {
    out << std::string(static_cast<std::size_t>(depth) * 2, ' ') << node->label << "\n";
    for (std::size_t i = 0; i < node->children.size(); ++i) {
        printNode(out, node->children[i], depth + 1);
    }
}

// ---------------------------------------------------------------------------
// 表驱动预测分析 + 建树 + 错误恢复。
// ---------------------------------------------------------------------------
// 符号栈元素，有两种：
//   Symbol：待展开的非终结符，或待匹配的终结符；
//   Reduce：规约标记，等右部所有符号处理完后触发建树。
// checkpointIndex 只对被保护的符号（Stmt / FuncDef）有意义，
// 规约到该符号时据此把对应检查点标记为失效。
struct StackItem {
    enum Kind { Symbol, Reduce } kind;
    std::string symbol;
    int productionId;
    int checkpointIndex;

    StackItem() : kind(Symbol), productionId(-1), checkpointIndex(-1) {}
};

// 错误恢复检查点。
// 目标是在语句或函数级别丢弃出错的部分，然后继续解析后面的内容。
// stackSize 记录“被保护符号”下方的符号栈大小，valueSize 记录对应的值栈大小；
// 恢复时截断到这两个位置，再补一个 ERROR 节点，父产生式的子节点个数仍然对得上。
struct Checkpoint {
    bool active;         // 已经被使用或对应符号已规约则为 false
    bool functionLevel;  // true 表示函数级恢复，false 表示语句级恢复
    std::size_t stackSize;
    std::size_t valueSize;
};

// 表驱动 LL(1) 分析器。
//   - 用分析表决定每个非终结符展开成哪条产生式；
//   - 用值栈 + Reduce 标记同步构造语法分析树；
//   - 用检查点做语句/函数级错误恢复，尽量报告多个错误而不是第一个就停。
class TreeParser {
public:
    TreeParser(const std::vector<Token>& tokens, bool trace)
        : tokens_(tokens), trace_(trace), index_(0) {}

    // 主循环。每一步处理栈顶：
    //   1) Reduce 标记：弹出子节点，组装成左部节点；
    //   2) 终结符且与向前看一致：匹配并消费一个 token；
    //   3) 非终结符：查 M[top, lookahead]，展开对应产生式；
    //   4) 否则是语法错误，进入恢复流程。
    void run(std::vector<Error>& errors) {
        errors_ = &errors;
        stack_.push_back(makeSymbol("$"));
        stack_.push_back(makeSymbol(ll1data::kStartSymbol));

        while (!stack_.empty()) {
            if (index_ >= tokens_.size()) {
                break;
            }
            if (stack_.back().kind == StackItem::Reduce) {
                reduce();
                continue;
            }

            const std::string top = stack_.back().symbol;
            const std::string lookahead = terminalOf(tokens_[index_]);

            // 栈顶是终结符：只做匹配，不查表。
            if (top == lookahead) {
                if (trace_) {
                    std::cerr << "match " << lookahead << "\n";
                }
                if (lookahead != "$") {
                    values_.push_back(makeLeafNode(lookahead, tokens_[index_].text));
                }
                stack_.pop_back();
                ++index_;
                continue;
            }

            // 栈顶是非终结符：用向前看终结符查嵌套字典，得到要展开的产生式。
            const int id = lookup(top, lookahead);
            if (id >= 0) {
                expand(id);
                continue;
            }

            // 查表失败：记录错误并尝试恢复。
            // 如果恢复后 token 下标和栈大小都没变化，强制前进一个 token，
            // 防止在同一位置反复报错导致死循环。
            const std::size_t beforeIndex = index_;
            const std::size_t beforeStack = stack_.size();
            handleError(top, lookahead);
            if (index_ == beforeIndex && stack_.size() == beforeStack &&
                !stack_.empty()) {
                if (index_ + 1 < tokens_.size()) {
                    ++index_;
                } else {
                    stack_.clear();
                }
            }
        }
    }

    // 把已经建好的树写到文件。
    // 正常结束时值栈里只剩一个根节点；恢复失败时可能有多个片段，
    // 这时输出 "(partial tree)" 再把它们逐个打印出来。
    bool writeTree(const std::string& path) const {
        std::ofstream out(path.c_str(), std::ios::binary);
        if (!out) {
            return false;
        }
        if (values_.empty()) {
            out << "(no tree)\n";
        } else if (values_.size() == 1) {
            printNode(out, values_[0], 0);
        } else {
            out << "(partial tree)\n";
            for (std::size_t i = 0; i < values_.size(); ++i) {
                printNode(out, values_[i], 1);
            }
        }
        return true;
    }

private:
    const std::vector<Token>& tokens_;  // 词法结果
    bool trace_;                        // -v：把过程打到 stderr
    std::size_t index_;                 // 当前向前看 token 下标
    std::vector<StackItem> stack_;      // 符号栈（含 Reduce 标记）
    std::vector<NodePtr> values_;       // 值栈：已完成的子树
    std::vector<Checkpoint> checkpoints_;  // 错误恢复检查点
    std::vector<Error>* errors_;        // 错误列表（与词法错误共用）

    // 构造一个待处理符号。
    StackItem makeSymbol(const std::string& symbol) const {
        StackItem item;
        item.kind = StackItem::Symbol;
        item.symbol = symbol;
        return item;
    }

    // 构造规约标记；checkpointIndex 用于在规约时使对应检查点失效。
    StackItem makeReduce(int id, int checkpointIndex) const {
        StackItem item;
        item.kind = StackItem::Reduce;
        item.productionId = id;
        item.checkpointIndex = checkpointIndex;
        return item;
    }

    // 按产生式 id 展开：
    //   弹出左部符号 → 压入 Reduce 标记 → 逆序压入右部。
    // 逆序压栈是为了让右部第一个符号位于栈顶，先被处理。
    void expand(int id) {
        const ll1data::Production& production = ll1data::kProductions[id - 1];
        const std::vector<std::string>& rhs = production.rhs;
        const std::string& lhs = production.lhs;

        const StackItem parent = stack_.back();
        stack_.pop_back();
        stack_.push_back(makeReduce(id, parent.checkpointIndex));

        for (std::vector<std::string>::const_reverse_iterator it = rhs.rbegin();
             it != rhs.rend(); ++it) {
            stack_.push_back(makeSymbol(*it));
        }

        // 语句列表和编译单元的展开处建立检查点：
        // 之后 Stmt / FuncDef 解析失败时，可从保存的位置丢弃并继续。
        const bool stmtGuard =
            (lhs == "StmtList" || lhs == "StmtList'") &&
            !rhs.empty() && rhs[0] == "Stmt";
        const bool funcGuard =
            (lhs == "CompUnit" || lhs == "CompUnit'") &&
            !rhs.empty() && rhs[0] == "FuncDef";

        if (stmtGuard || funcGuard) {
            Checkpoint checkpoint;
            checkpoint.active = true;
            checkpoint.functionLevel = funcGuard;
            checkpoint.stackSize = stack_.size() - 1;  // 不含被保护的 Stmt/FuncDef
            checkpoint.valueSize = values_.size();
            checkpoints_.push_back(checkpoint);
            // 把检查点编号挂到栈顶的被保护符号上；
            // 该符号展开时编号会传给它的 Reduce 标记，规约时用于失效。
            stack_.back().checkpointIndex =
                static_cast<int>(checkpoints_.size()) - 1;
        }

        if (trace_) {
            std::cerr << "expand " << production.text << "\n";
        }
    }

    // 规约：从值栈弹出右部对应的子节点，组装成左部节点压回。
    // 弹出顺序是反的，所以要 reverse；空产生式额外加一个 ε 子节点。
    void reduce() {
        const StackItem item = stack_.back();
        stack_.pop_back();
        const int id = item.productionId;
        const ll1data::Production& production = ll1data::kProductions[id - 1];
        const std::vector<std::string>& rhs = production.rhs;
        const std::string& lhs = production.lhs;

        // 被保护符号正常规约完成，对应检查点不再需要。
        if (item.checkpointIndex >= 0 &&
            item.checkpointIndex < static_cast<int>(checkpoints_.size())) {
            checkpoints_[static_cast<std::size_t>(item.checkpointIndex)].active = false;
        }

        std::vector<NodePtr> children;
        for (std::size_t i = 0; i < rhs.size(); ++i) {
            if (!values_.empty()) {
                children.push_back(values_.back());
                values_.pop_back();
            } else {
                // 理论上不会发生；真遇到时用错误节点补齐，保证不破坏父节点结构。
                children.push_back(makeErrorNode(0));
            }
        }
        std::reverse(children.begin(), children.end());

        NodePtr node = makeNode(lhs);
        if (rhs.empty()) {
            node->children.push_back(makeNode("ε"));
        } else {
            node->children = children;
        }
        values_.push_back(node);

        if (trace_) {
            std::cerr << "reduce " << production.text << "\n";
        }
    }

    // 从后往前找最近的活动检查点，也就是最内层还没恢复过的语句/函数。
    int findLastActiveCheckpoint() const {
        for (int i = static_cast<int>(checkpoints_.size()) - 1; i >= 0; --i) {
            if (checkpoints_[static_cast<std::size_t>(i)].active) {
                return i;
            }
        }
        return -1;
    }

    // 对应 错误处理.txt 的 report_error(expect, actual, line, col)。
    // expected 由当前栈顶决定：
    //   非终结符 -> 它在 kTable 里的所有列，也就是该行的期望集合；
    //   终结符   -> 它自己。
    // stdout 之后只打印 line；完整四元组在 -v 时打到 stderr。
    void reportError(const std::string& top, const std::string& actual,
                     const Token& token) {
        std::string expected;
        std::map<std::string, std::map<std::string, int> >::const_iterator row =
            ll1data::kTable.find(top);
        if (row != ll1data::kTable.end()) {
            for (std::map<std::string, int>::const_iterator it = row->second.begin();
                 it != row->second.end(); ++it) {
                if (!expected.empty()) {
                    expected += " | ";
                }
                expected += it->first;
            }
        } else {
            expected = top;
        }

        // EOF 表示缺失符号时，错误位置归到最后一个源代码行，
        // 与 cases20261110 的行号约定一致。
        const int errorLine =
            actual == "$" && token.line > 0 ? token.line - 1 : token.line;
        errors_->push_back(Error{errorLine, token.column, expected, actual});
        if (trace_) {
            std::cerr << "error at line " << errorLine << ", column "
                      << token.column << ": expected " << expected
                      << ", got " << actual << "\n";
        }
    }

    // 对应 错误处理.txt 的 Panic-Mode 同步：
    //   sync = { ';', '}', if, while, return }
    // 丢弃 token 直到遇到当前块层级的同步符；'}' 留给外层块消费。
    // 返回 false 表示一直到 EOF 都没有找到同步符。
    bool synchronize() {
        int depth = 0;
        while (index_ < tokens_.size()) {
            const std::string& text = tokens_[index_].text;
            if (text == "<eof>") {
                return false;
            }
            if (text == "{") {
                ++depth;
                ++index_;
                continue;
            }
            if (text == "}") {
                if (depth == 0) {
                    return true;
                }
                --depth;
                ++index_;
                continue;
            }
            if (depth == 0 &&
                (text == "if" || text == "while" || text == "return")) {
                return true;
            }
            if (text == ";") {
                ++index_;  // 同步到 ';' 时消费它
                return true;
            }
            ++index_;
        }
        return false;
    }

    // 截断到检查点，再用 ERROR 节点占住被丢弃符号的位置，
    // 这样父产生式仍能正常规约，树结构不会错位。
    void recoverAtCheckpoint(int checkpointIndex, const Token& token,
                             bool skipFunction) {
        Checkpoint& checkpoint = checkpoints_[static_cast<std::size_t>(checkpointIndex)];
        if (checkpoint.stackSize < stack_.size()) {
            stack_.resize(checkpoint.stackSize);
        }
        if (checkpoint.valueSize < values_.size()) {
            values_.resize(checkpoint.valueSize);
        }
        values_.push_back(makeErrorNode(token.line));
        checkpoint.active = false;
        if (skipFunction) {
            skipFunctionRemainder();
        }
    }

    // 语法错误恢复（Panic-Mode）：
    //   1) 按文档记录 expect/actual/line/col；
    //   2) 按同步集合丢弃 token；
    //   3) ';' 消费，'}' 留给外层块，控制关键字处开始解析后续语句；
    //   4) 语句级错误截断到语句检查点，函数级错误丢弃整个函数。
    void handleError(const std::string& top, const std::string& lookahead) {
        const Token& token = tokens_[index_];

        // 函数体内已经发生语法错误时，后续 token 不应被误判为新的函数定义。
        if (top == "CompUnit'" && errors_ != nullptr && !errors_->empty()) {
            stack_.clear();
            return;
        }

        reportError(top, lookahead, token);

        const int checkpointIndex = findLastActiveCheckpoint();

        // 语句列表位置：恢复层级就是 StmtList/StmtList' 本身。
        // 同步后直接让列表继续解析下一条语句，或走 ε 等待 '}' 闭合。
        if (top == "StmtList" || top == "StmtList'") {
            if (synchronize()) {
                return;
            }
            // 到 EOF 都没同步上，交给下面的函数级兜底。
        } else if (checkpointIndex >= 0 &&
                   checkpoints_[static_cast<std::size_t>(checkpointIndex)].functionLevel) {
            // 函数头等函数级错误：语句同步集合不适用，直接丢弃整个函数。
            recoverAtCheckpoint(checkpointIndex, token, true);
            return;
        } else {
            synchronize();
        }

        if (checkpointIndex < 0) {
            // 没有检查点，无法安全恢复：记录后结束。
            stack_.clear();
            return;
        }

        const bool functionLevel =
            checkpoints_[static_cast<std::size_t>(checkpointIndex)].functionLevel;
        recoverAtCheckpoint(checkpointIndex, token, functionLevel);
    }

    // 函数级恢复：先找到函数体的 '{'，再按花括号配对跳到函数结束。
    // 如果错误发生在函数头，这样能整体丢掉这个函数；
    // 如果没有找到 '{'（例如文件在函数中途结束），直接走到 EOF。
    void skipFunctionRemainder() {
        while (index_ < tokens_.size() && tokens_[index_].text != "{" &&
               tokens_[index_].text != "<eof>") {
            ++index_;
        }
        if (index_ >= tokens_.size() || tokens_[index_].text == "<eof>") {
            return;
        }
        int depth = 0;
        do {
            if (tokens_[index_].text == "{") {
                ++depth;
            } else if (tokens_[index_].text == "}") {
                --depth;
            }
            ++index_;
        } while (index_ < tokens_.size() && depth > 0 &&
                 tokens_[index_].text != "<eof>");
    }
};

// 程序入口：
//   TableParser.exe [-v]
// 源码从 stdin 读取；stdout 只输出 accept/reject 和错误行号。
int main(int argc, char** argv) {
    bool trace = false;
    // -q 保留为兼容旧用法。
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-ast") {
            trace = false;
        } else if (arg == "-q" || arg == "--quiet") {
            // 默认就是安静模式，保留该参数以兼容旧用法。
        } else {
            return 2;
        }
    }

    try {
        // 1) 从标准输入读完整源码。
        const std::string source((std::istreambuf_iterator<char>(std::cin)),
                                 std::istreambuf_iterator<char>());

        // 2) 词法分析。词法错误直接进入 errors，最后和语法错误一起输出。
        std::vector<Error> errors;
        const std::vector<Token> tokens = lexShared(source, errors);

        // 3) 表驱动语法分析并建树（分析表已在编译期嵌入）。
        TreeParser parser(tokens, trace);
        parser.run(errors);

        // 5) 无论成功失败都尝试写出语法树，方便调试。
        parser.writeTree("tree.txt");
        // 6) 按任务要求输出：accept 一行；否则 reject + 每行一个错误行号。
        if (errors.empty()) {
            std::cout << "accept\n";
        } else {
            std::cout << "reject\n";
            std::set<int> reportedLines;
            for (std::size_t i = 0; i < errors.size(); ++i) {
                reportedLines.insert(errors[i].line);
            }
            for (std::set<int>::const_iterator it = reportedLines.begin();
                 it != reportedLines.end(); ++it) {
                std::cout << *it << "\n";
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 2;
    }
}
