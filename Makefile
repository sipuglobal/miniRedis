# Mini Redis Makefile

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g   # -g 保留调试信息，供 LLDB 使用
TARGET   = mini_redis
SRCS     = main.cpp server.cpp command.cpp database.cpp
OBJS     = $(SRCS:.cpp=.o)

# 默认目标：编译生成可执行文件
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译每个 .cpp 为 .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译产物
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
