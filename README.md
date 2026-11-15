# ToyC 编译器阶段性提交

当前提交包含词法分析、递归下降语法分析和语义检查，并通过 `-ast` 接口输出检查结果。代码生成和汇编输出尚未实现。

## 构建

```sh
cmake -S . -B build
cmake --build build
```

也可以直接编译：

```sh
g++ -std=c++11 -Wall -Wextra -Wpedantic src/Parser.cpp -o compiler
```

## 使用

```sh
./build/compiler -ast < input.c > input.check-ast
```

程序从标准输入读取 ToyC 源码，在标准输出写入 `accept` 或 `reject` 及错误行号，并在当前目录生成 `tree.txt`。

`-asm` 和 `-opt` 暂未实现；本阶段不提交代码生成和运行时库。

## 目录

- `src/Parser.cpp`：词法、递归下降语法分析、语义检查和语法树输出。
- `src/Lexer.h`、`src/Lexer.cpp`：共享词法模块，供后续统一接入。
- `grammar/`：当前 BNF 文法、LL(1) 表数据和错误恢复说明。
