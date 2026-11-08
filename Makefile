# 1. 自动查找当前目录下的第一个 .cpp 文件
SRC = $(firstword $(wildcard *.cpp))

# 2. 提取不带后缀的基本文件名 (例如: my_program)
BIN = $(basename $(SRC))

# 3. 定义目标 token 文件名 (例如: my_program.token)
TOKEN_FILE = $(BIN).token

# 默认目标
all: $(TOKEN_FILE)

# 核心规则：生成 .token 文件
$(TOKEN_FILE): $(SRC)
	g++ -Wall -std=c++17 $(SRC) -o $(BIN)
	./$(BIN) > $(TOKEN_FILE)

# 清理生成的文件
clean:
	rm -f $(BIN) $(TOKEN_FILE)

.PHONY: all clean