# ==========================================================
# ToyC 词法 / 语法 / 语义分析器 Makefile
#
# 用法：
#   mingw32-make            # Windows + MinGW 构建
#   make                    # Linux / macOS 构建
#   mingw32-make clean      # 删除 Syntax.exe
#   mingw32-make rebuild    # 先清理再完整构建
#
# 说明：
#   - 评测平台按 ./Syntax.exe 调用，因此目标固定为 Syntax.exe。
#   - 编译 Syntax.cpp；LL1TableData.h 由它 #include，
#     所以 .h 作为依赖列出，不单独编译。
#   - Windows 下的编译器与 .vscode/tasks.json 保持一致。
#   - 依赖 FORCE，每次 make 都重新编译：即使仓库里带着
#     Windows 的 Syntax.exe，评测机（Linux）执行 make 时也会
#     重新生成当前平台的可执行文件。
# ==========================================================

ifeq ($(OS),Windows_NT)
MINGW   := C:/Users/leglon/x86_64-16.1.0-release-win32-seh-ucrt-rt_v14-rev1/mingw64
CXX     := $(MINGW)/bin/g++.exe
LDFLAGS := -static
RM_F    := cmd /c del /Q /F
else
CXX     := g++
LDFLAGS :=
RM_F    := rm -f
endif

CXXFLAGS := -fdiagnostics-color=always -g

TARGET := Syntax.exe
SRC    := Syntax.cpp
HDR    := LL1TableData.h

.PHONY: all clean rebuild FORCE

all: $(TARGET)

# 目标名与评测平台一致；FORCE 保证每次都重新链接
$(TARGET): $(SRC) $(HDR) FORCE
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDFLAGS)

FORCE:

clean:
	-$(RM_F) "$(TARGET)"

rebuild: clean all
