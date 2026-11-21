# ==========================================================
# ToyC 词法 / 语法 / 语义分析器 Makefile
#
# 用法：
#   mingw32-make            # Windows + MinGW 构建
#   make                    # Linux / macOS 构建
#   mingw32-make clean      # 删除中间文件和可执行文件
#   mingw32-make rebuild    # 先清理再完整构建
#
# 说明：
#   LL1TableData.h 不单独编译，它通过 #include 参与编译，
#   因此作为 syntax_semantic.o 的依赖参与增量构建。
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

OBJ := syntax_semantic.o
SRC := Syntax\ Analysis\ and\ Semantic\ Analysis.cpp
HDR := LL1TableData.h

# Windows 没有 rm，用 cmd 的 del；Linux/macOS 用 rm
ifeq ($(OS),Windows_NT)
RM_F := cmd /c del /Q /F
else
RM_F := rm -f
endif

.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) -o "$@" "$(OBJ)"

$(OBJ): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

clean:
	-$(RM_F) "$(OBJ)"
	-$(RM_F) "$(TARGET_NAME)"

rebuild: clean all
