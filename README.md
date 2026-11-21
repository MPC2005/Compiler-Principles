# ToyC 词法分析器

本项目实现 ToyC 词法分析：从标准输入读取源代码，识别并输出 token。当前提交仅包含词法分析阶段。

## 构建

需要 CMake 3.16 或更高版本和支持 C++11 的编译器：

```sh
cmake -S . -B build
cmake --build build
```

也可直接编译：

```sh
g++ -std=c++11 -Wall -Wextra -Wpedantic src/Tokenizer.cpp src/Lexer.cpp -o compiler
```

## 运行

```sh
./build/compiler < input.c > input.token
```

程序逐行输出 token，序号从 0 开始，例如：

```text
0:'int':"int"
1:Ident:"main"
```

遇到非法字符时，程序先输出 `reject` 和错误行号及原因，再输出空行和继续扫描得到的 token。错误行号从 0 开始；词法模式报告错误后仍返回退出码 0。

## 文件

- `src/Tokenizer.cpp`：程序入口和输出。
- `src/Lexer.h`、`src/Lexer.cpp`：token 定义及词法扫描实现。
- `CMakeLists.txt`：构建配置。

