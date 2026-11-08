CXX = g++
CXXFLAGS = -Wall -std=c++17 -MMD -MP  # -MMD生成依赖，-MP防止头文件删除导致的错误
TARGET = my_program
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)  # 依赖文件列表

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 将生成的.d文件包含进来，使Make知道头文件依赖
-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

.PHONY: all clean