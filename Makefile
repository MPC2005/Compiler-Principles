# 编译器与标志
CC := gcc
CFLAGS := -Wall -O2
TARGET := myapp

# 自动查找源文件并生成对象文件列表
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

# 默认目标
all: $(TARGET)

# 链接规则：将所有 .o 文件链接成可执行文件
$(TARGET): $(OBJS)
	$(CC) $^ -o $@

# 模式规则：利用内置规则自动将 .c 编译为 .o
# 这里显式列出以覆盖默认的编译命令，便于演示
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 伪目标：清理生成的文件
.PHONY: all clean
clean:
	rm -f $(OBJS) $(TARGET)
