# ToyC 语法分析器

本项目将词法分析与表驱动 LL(1) 语法分析集成，读取 ToyC 源码，检查语法并生成便于调试的 `tree.txt`。本阶段不进行语义检查。文法中的悬空 `else` 按最近的未匹配 `if` 结合。

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

检查通过时标准输出为 `accept`；失败时输出 `reject`，随后按升序逐行输出零基错误行号，同一行只输出一次。两种检查结果的退出码均为 0。程序在当前工作目录生成 `tree.txt`；异常诊断写入标准错误。

## 文件

- `src/Lexer.h`、`src/Lexer.cpp`：词法分析。
- `src/ParserLL1.cpp`：表驱动语法分析、错误恢复、语法树和命令行入口。
- `src/LL1TableData.h`：编译时使用的预测分析表和产生式。
- `CMakeLists.txt`：构建配置。

语法错误采用 panic-mode 恢复，在语句同步符附近继续分析。

