# ToyC 递归下降分析器

程序使用递归下降法进行词法和语法分析。语法通过后检查变量作用域、重复声明、函数调用、返回值和循环控制语句等语义规则。

## 构建

需要 CMake 3.16 或更高版本及支持 C++11 的编译器：

```sh
cmake -S . -B build
cmake --build build
```

## 运行

```sh
./build/compiler -ast < input.c > input.check-ast
```

程序从标准输入读取源码。检查通过输出 `accept`；否则输出 `reject` 和按升序排列的零基错误行号，同一行只输出一次。两种检查结果退出码均为 0。

程序在运行目录生成 `tree.txt`。`ASTtree.txt` 输出暂时关闭，AST 节点与构造代码保留供后续使用。

## 文件

- `src/Lexer.h`、`src/Lexer.cpp`：复用第一部分的词法分析模块。
- `src/ParserDescent.cpp`：词法结果适配、递归下降语法分析、语义检查和 AST 构造。
- `src/tree.h`、`src/tree.cpp`：AST 节点结构及调试输出。
- `CMakeLists.txt`：构建配置。
