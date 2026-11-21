// 本文件由 tools/gen_table_header.py 自动生成，请勿手工修改。
// 数据源：LL(1)分析表_BNF文法.json
// 修改文法或分析表后，重新运行：python tools/gen_table_header.py

#ifndef TOYC_LL1_TABLE_DATA_H
#define TOYC_LL1_TABLE_DATA_H

#include <map>
#include <set>
#include <string>
#include <vector>

namespace ll1data {

static const char* const kStartSymbol = "CompUnit";

// 嵌套字典：kTable[非终结符][向前看终结符] = 产生式 id（从 1 开始）。
// 不存在的格子就是 error。
static const std::map<std::string, std::map<std::string, int> > kTable = {
    {"CompUnit", {{"int", 1}, {"void", 1}}},
    {"CompUnit'", {{"int", 2}, {"void", 2}, {"$", 3}}},
    {"FuncDef", {{"int", 4}, {"void", 4}}},
    {"FuncType", {{"int", 5}, {"void", 6}}},
    {"ParamOpt", {{"int", 7}, {")", 8}}},
    {"ParamList", {{"int", 9}}},
    {"ParamList'", {{")", 11}, {",", 10}}},
    {"Param", {{"int", 12}}},
    {"Block", {{"{", 13}}},
    {"StmtList", {
        {"ID", 14}, {"NUMBER", 14}, {"int", 14}, {"const", 14}, {"if", 14}, {"while", 14},
        {"break", 14}, {"continue", 14}, {"return", 14}, {"(", 14}, {"{", 14}, {"}", 15}, {";", 14},
        {"+", 14}, {"-", 14}, {"!", 14},
    }},
    {"StmtList'", {
        {"ID", 16}, {"NUMBER", 16}, {"int", 16}, {"const", 16}, {"if", 16}, {"while", 16},
        {"break", 16}, {"continue", 16}, {"return", 16}, {"(", 16}, {"{", 16}, {"}", 17}, {";", 16},
        {"+", 16}, {"-", 16}, {"!", 16},
    }},
    {"Stmt", {
        {"ID", 18}, {"NUMBER", 19}, {"int", 22}, {"const", 23}, {"if", 24}, {"while", 25},
        {"break", 26}, {"continue", 27}, {"return", 28}, {"(", 19}, {"{", 20}, {";", 21}, {"+", 19},
        {"-", 19}, {"!", 19},
    }},
    {"IDStmtTail", {
        {"(", 30}, {";", 30}, {"=", 29}, {"||", 30}, {"&&", 30}, {"<", 30}, {">", 30}, {"<=", 30},
        {">=", 30}, {"==", 30}, {"!=", 30}, {"+", 30}, {"-", 30}, {"*", 30}, {"/", 30}, {"%", 30},
    }},
    {"InitOpt", {{";", 32}, {"=", 31}}},
    {"ElseOpt", {
        {"ID", 34}, {"NUMBER", 34}, {"int", 34}, {"const", 34}, {"if", 34}, {"else", 33},
        {"while", 34}, {"break", 34}, {"continue", 34}, {"return", 34}, {"(", 34}, {"{", 34},
        {"}", 34}, {";", 34}, {"+", 34}, {"-", 34}, {"!", 34},
    }},
    {"ReturnOpt", {
        {"ID", 35}, {"NUMBER", 35}, {"(", 35}, {";", 36}, {"+", 35}, {"-", 35}, {"!", 35},
    }},
    {"Expr", {{"ID", 37}, {"NUMBER", 37}, {"(", 37}, {"+", 37}, {"-", 37}, {"!", 37}}},
    {"LOrExpr", {{"ID", 38}, {"NUMBER", 38}, {"(", 38}, {"+", 38}, {"-", 38}, {"!", 38}}},
    {"LOrExpr'", {{")", 40}, {";", 40}, {",", 40}, {"||", 39}}},
    {"LAndExpr", {{"ID", 41}, {"NUMBER", 41}, {"(", 41}, {"+", 41}, {"-", 41}, {"!", 41}}},
    {"LAndExpr'", {{")", 43}, {";", 43}, {",", 43}, {"||", 43}, {"&&", 42}}},
    {"RelExpr", {{"ID", 44}, {"NUMBER", 44}, {"(", 44}, {"+", 44}, {"-", 44}, {"!", 44}}},
    {"RelExpr'", {
        {")", 46}, {";", 46}, {",", 46}, {"||", 46}, {"&&", 46}, {"<", 45}, {">", 45}, {"<=", 45},
        {">=", 45}, {"==", 45}, {"!=", 45},
    }},
    {"RelOp", {{"<", 47}, {">", 48}, {"<=", 49}, {">=", 50}, {"==", 51}, {"!=", 52}}},
    {"AddExpr", {{"ID", 53}, {"NUMBER", 53}, {"(", 53}, {"+", 53}, {"-", 53}, {"!", 53}}},
    {"AddExpr'", {
        {")", 55}, {";", 55}, {",", 55}, {"||", 55}, {"&&", 55}, {"<", 55}, {">", 55}, {"<=", 55},
        {">=", 55}, {"==", 55}, {"!=", 55}, {"+", 54}, {"-", 54},
    }},
    {"AddOp", {{"+", 56}, {"-", 57}}},
    {"MulExpr", {{"ID", 58}, {"NUMBER", 58}, {"(", 58}, {"+", 58}, {"-", 58}, {"!", 58}}},
    {"MulExpr'", {
        {")", 60}, {";", 60}, {",", 60}, {"||", 60}, {"&&", 60}, {"<", 60}, {">", 60}, {"<=", 60},
        {">=", 60}, {"==", 60}, {"!=", 60}, {"+", 60}, {"-", 60}, {"*", 59}, {"/", 59}, {"%", 59},
    }},
    {"MulOp", {{"*", 61}, {"/", 62}, {"%", 63}}},
    {"UnaryExpr", {{"ID", 64}, {"NUMBER", 64}, {"(", 64}, {"+", 65}, {"-", 66}, {"!", 67}}},
    {"PrimaryExpr", {{"ID", 68}, {"NUMBER", 69}, {"(", 70}}},
    {"PrimaryExprTail", {
        {"(", 71}, {")", 72}, {";", 72}, {",", 72}, {"||", 72}, {"&&", 72}, {"<", 72}, {">", 72},
        {"<=", 72}, {">=", 72}, {"==", 72}, {"!=", 72}, {"+", 72}, {"-", 72}, {"*", 72}, {"/", 72},
        {"%", 72},
    }},
    {"ArgOpt", {{"ID", 73}, {"NUMBER", 73}, {"(", 73}, {")", 74}, {"+", 73}, {"-", 73}, {"!", 73}}},
    {"ArgList", {{"ID", 75}, {"NUMBER", 75}, {"(", 75}, {"+", 75}, {"-", 75}, {"!", 75}}},
    {"ArgList'", {{")", 77}, {",", 76}}},
    {"NonIdExpr", {{"NUMBER", 78}, {"(", 78}, {"+", 78}, {"-", 78}, {"!", 78}}},
    {"NonIdUnary", {{"NUMBER", 79}, {"(", 79}, {"+", 80}, {"-", 81}, {"!", 82}}},
    {"NonIdPrimary", {{"NUMBER", 83}, {"(", 84}}},
};

// 产生式：下标 id-1 对应 kTable 里的 id；rhs 为空表示 ε。
struct Production {
    std::string lhs;
    std::vector<std::string> rhs;
    std::string text;
};

static const std::vector<Production> kProductions = {
    {"CompUnit", {"FuncDef", "CompUnit'"}, "CompUnit -> FuncDef CompUnit'"},
    {"CompUnit'", {"FuncDef", "CompUnit'"}, "CompUnit' -> FuncDef CompUnit'"},
    {"CompUnit'", {}, "CompUnit' -> ε"},
    {"FuncDef", {"FuncType", "ID", "(", "ParamOpt", ")", "Block"}, "FuncDef -> FuncType ID ( ParamOpt ) Block"},
    {"FuncType", {"int"}, "FuncType -> int"},
    {"FuncType", {"void"}, "FuncType -> void"},
    {"ParamOpt", {"ParamList"}, "ParamOpt -> ParamList"},
    {"ParamOpt", {}, "ParamOpt -> ε"},
    {"ParamList", {"Param", "ParamList'"}, "ParamList -> Param ParamList'"},
    {"ParamList'", {",", "Param", "ParamList'"}, "ParamList' -> , Param ParamList'"},
    {"ParamList'", {}, "ParamList' -> ε"},
    {"Param", {"int", "ID"}, "Param -> int ID"},
    {"Block", {"{", "StmtList", "}"}, "Block -> { StmtList }"},
    {"StmtList", {"Stmt", "StmtList'"}, "StmtList -> Stmt StmtList'"},
    {"StmtList", {}, "StmtList -> ε"},
    {"StmtList'", {"Stmt", "StmtList'"}, "StmtList' -> Stmt StmtList'"},
    {"StmtList'", {}, "StmtList' -> ε"},
    {"Stmt", {"ID", "IDStmtTail"}, "Stmt -> ID IDStmtTail"},
    {"Stmt", {"NonIdExpr", ";"}, "Stmt -> NonIdExpr ;"},
    {"Stmt", {"Block"}, "Stmt -> Block"},
    {"Stmt", {";"}, "Stmt -> ;"},
    {"Stmt", {"int", "ID", "InitOpt", ";"}, "Stmt -> int ID InitOpt ;"},
    {"Stmt", {"const", "int", "ID", "=", "Expr", ";"}, "Stmt -> const int ID = Expr ;"},
    {"Stmt", {"if", "(", "Expr", ")", "Stmt", "ElseOpt"}, "Stmt -> if ( Expr ) Stmt ElseOpt"},
    {"Stmt", {"while", "(", "Expr", ")", "Stmt"}, "Stmt -> while ( Expr ) Stmt"},
    {"Stmt", {"break", ";"}, "Stmt -> break ;"},
    {"Stmt", {"continue", ";"}, "Stmt -> continue ;"},
    {"Stmt", {"return", "ReturnOpt", ";"}, "Stmt -> return ReturnOpt ;"},
    {"IDStmtTail", {"=", "Expr", ";"}, "IDStmtTail -> = Expr ;"},
    {"IDStmtTail", {"PrimaryExprTail", "MulExpr'", "AddExpr'", "RelExpr'", "LAndExpr'", "LOrExpr'", ";"}, "IDStmtTail -> PrimaryExprTail MulExpr' AddExpr' RelExpr' LAndExpr' LOrExpr' ;"},
    {"InitOpt", {"=", "Expr"}, "InitOpt -> = Expr"},
    {"InitOpt", {}, "InitOpt -> ε"},
    {"ElseOpt", {"else", "Stmt"}, "ElseOpt -> else Stmt"},
    {"ElseOpt", {}, "ElseOpt -> ε"},
    {"ReturnOpt", {"Expr"}, "ReturnOpt -> Expr"},
    {"ReturnOpt", {}, "ReturnOpt -> ε"},
    {"Expr", {"LOrExpr"}, "Expr -> LOrExpr"},
    {"LOrExpr", {"LAndExpr", "LOrExpr'"}, "LOrExpr -> LAndExpr LOrExpr'"},
    {"LOrExpr'", {"||", "LAndExpr", "LOrExpr'"}, "LOrExpr' -> || LAndExpr LOrExpr'"},
    {"LOrExpr'", {}, "LOrExpr' -> ε"},
    {"LAndExpr", {"RelExpr", "LAndExpr'"}, "LAndExpr -> RelExpr LAndExpr'"},
    {"LAndExpr'", {"&&", "RelExpr", "LAndExpr'"}, "LAndExpr' -> && RelExpr LAndExpr'"},
    {"LAndExpr'", {}, "LAndExpr' -> ε"},
    {"RelExpr", {"AddExpr", "RelExpr'"}, "RelExpr -> AddExpr RelExpr'"},
    {"RelExpr'", {"RelOp", "AddExpr", "RelExpr'"}, "RelExpr' -> RelOp AddExpr RelExpr'"},
    {"RelExpr'", {}, "RelExpr' -> ε"},
    {"RelOp", {"<"}, "RelOp -> <"},
    {"RelOp", {">"}, "RelOp -> >"},
    {"RelOp", {"<="}, "RelOp -> <="},
    {"RelOp", {">="}, "RelOp -> >="},
    {"RelOp", {"=="}, "RelOp -> =="},
    {"RelOp", {"!="}, "RelOp -> !="},
    {"AddExpr", {"MulExpr", "AddExpr'"}, "AddExpr -> MulExpr AddExpr'"},
    {"AddExpr'", {"AddOp", "MulExpr", "AddExpr'"}, "AddExpr' -> AddOp MulExpr AddExpr'"},
    {"AddExpr'", {}, "AddExpr' -> ε"},
    {"AddOp", {"+"}, "AddOp -> +"},
    {"AddOp", {"-"}, "AddOp -> -"},
    {"MulExpr", {"UnaryExpr", "MulExpr'"}, "MulExpr -> UnaryExpr MulExpr'"},
    {"MulExpr'", {"MulOp", "UnaryExpr", "MulExpr'"}, "MulExpr' -> MulOp UnaryExpr MulExpr'"},
    {"MulExpr'", {}, "MulExpr' -> ε"},
    {"MulOp", {"*"}, "MulOp -> *"},
    {"MulOp", {"/"}, "MulOp -> /"},
    {"MulOp", {"%"}, "MulOp -> %"},
    {"UnaryExpr", {"PrimaryExpr"}, "UnaryExpr -> PrimaryExpr"},
    {"UnaryExpr", {"+", "UnaryExpr"}, "UnaryExpr -> + UnaryExpr"},
    {"UnaryExpr", {"-", "UnaryExpr"}, "UnaryExpr -> - UnaryExpr"},
    {"UnaryExpr", {"!", "UnaryExpr"}, "UnaryExpr -> ! UnaryExpr"},
    {"PrimaryExpr", {"ID", "PrimaryExprTail"}, "PrimaryExpr -> ID PrimaryExprTail"},
    {"PrimaryExpr", {"NUMBER"}, "PrimaryExpr -> NUMBER"},
    {"PrimaryExpr", {"(", "Expr", ")"}, "PrimaryExpr -> ( Expr )"},
    {"PrimaryExprTail", {"(", "ArgOpt", ")"}, "PrimaryExprTail -> ( ArgOpt )"},
    {"PrimaryExprTail", {}, "PrimaryExprTail -> ε"},
    {"ArgOpt", {"ArgList"}, "ArgOpt -> ArgList"},
    {"ArgOpt", {}, "ArgOpt -> ε"},
    {"ArgList", {"Expr", "ArgList'"}, "ArgList -> Expr ArgList'"},
    {"ArgList'", {",", "Expr", "ArgList'"}, "ArgList' -> , Expr ArgList'"},
    {"ArgList'", {}, "ArgList' -> ε"},
    {"NonIdExpr", {"NonIdUnary", "MulExpr'", "AddExpr'", "RelExpr'", "LAndExpr'", "LOrExpr'"}, "NonIdExpr -> NonIdUnary MulExpr' AddExpr' RelExpr' LAndExpr' LOrExpr'"},
    {"NonIdUnary", {"NonIdPrimary"}, "NonIdUnary -> NonIdPrimary"},
    {"NonIdUnary", {"+", "UnaryExpr"}, "NonIdUnary -> + UnaryExpr"},
    {"NonIdUnary", {"-", "UnaryExpr"}, "NonIdUnary -> - UnaryExpr"},
    {"NonIdUnary", {"!", "UnaryExpr"}, "NonIdUnary -> ! UnaryExpr"},
    {"NonIdPrimary", {"NUMBER"}, "NonIdPrimary -> NUMBER"},
    {"NonIdPrimary", {"(", "Expr", ")"}, "NonIdPrimary -> ( Expr )"},
};

// 终结符（不含 $；$ 由程序单独处理）。
static const std::set<std::string> kTerminals = {
    "ID", "NUMBER", "int", "void", "const", "if", "else", "while", "break", "continue", "return",
    "(", ")", "{", "}", ";", ",", "=", "||", "&&", "<", ">", "<=", ">=", "==", "!=", "+", "-", "*",
    "/", "%", "!",
};

}  // namespace ll1data

#endif  // TOYC_LL1_TABLE_DATA_H
