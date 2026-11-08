# 1. 定义变量
CXX = g++
CXXFLAGS = -Wall -std=c++17 -O2
TARGET = tokenizer

# 自动查找当前目录下所有的 .c 文件作为输入文件
INPUT_FILES = $(wildcard *.c)
# 根据输入文件，自动推导生成对应的 .token 输出文件列表
OUTPUT_FILES = $(INPUT_FILES:.c=.token)

# 2. 默认目标
all: $(TARGET) $(OUTPUT_FILES)

# 3. 编译 C++ 程序
$(TARGET): lexerc.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# 4. 核心规则：将 .c 文件作为输入，生成 .token 文件
# $< 代表第一个依赖（即 %.c 文件）
# $@ 代表目标文件（即 %.token 文件）
# 注意：这里依赖了 $(TARGET)，意味着如果C++代码变了，会先重新编译，再重新生成所有token
%.token: %.c $(TARGET)
	./$(TARGET) < $< > $@

# 5. 清理生成的文件
clean:
	rm -f $(TARGET) $(OUTPUT_FILES)

# 声明伪目标
.PHONY: all clean