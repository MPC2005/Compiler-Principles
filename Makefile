# ==========================================================
# ToyC 词法 / 语法 / 语义分析器 Makefile
#
# 用法：
#   mingw32-make            # Windows + MinGW 构建
#   make                    # Linux / macOS 构建
#   mingw32-make clean      # 删除可执行文件
#   mingw32-make rebuild    # 先清理再完整构建
#
# 说明：
#   构建只涉及以下两个文件：
#     - Syntax Analysis and Semantic Analysis.cpp  （编译单元）
#     - LL1TableData.h                              （被 .cpp include 的头文件）
#   头文件不单独编译，因此 g++ 命令中只出现 .cpp；
#   .h 作为依赖列出，修改 .h 或 .cpp 都会重新编译。
#   源文件名含空格，下面的反斜杠转义需要 GNU Make 4.4+。
# ==========================================================

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2
LDFLAGS  ?= -static

ifeq ($(OS),Windows_NT)
TARGET      := Syntax\ Analysis\ and\ Semantic\ Analysis.exe
TARGET_NAME := Syntax Analysis and Semantic Analysis.exe
else
TARGET      := Syntax\ Analysis\ and\ Semantic\ Analysis
TARGET_NAME := Syntax Analysis and Semantic Analysis
endif

# 参与编译的两个文件
HDR      := LL1TableData.h
SRC      := Syntax\ Analysis\ and\ Semantic\ Analysis.cpp
SRC_NAME := Syntax Analysis and Semantic Analysis.cpp

# Windows 没有 rm，用 cmd 的 del；Linux/macOS 用 rm
ifeq ($(OS),Windows_NT)
RM_F := cmd /c del /Q /F
else
RM_F := rm -f
endif

.PHONY: all clean rebuild

all: $(TARGET)

# 一次编译并链接；.h 由 .cpp include，因此不单独作为编译单元
$(TARGET): $(HDR) $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) "$(SRC_NAME)" -o "$@"

clean:
	-$(RM_F) "$(TARGET_NAME)"

rebuild: clean all
