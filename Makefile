# Makefile for MiniFS
# 支持 Windows MinGW 和 Linux GCC

CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
STATIC_FLAGS = -static -static-libgcc -static-libstdc++
TARGET = minifs
SOURCES = main.cpp minifs.cpp fs_tests.cpp shell_utils.cpp user.cpp

# Windows 特定设置
ifeq ($(OS),Windows_NT)
    TARGET = minifs.exe
    # 尝试检测 MinGW 路径
    ifdef MINGW_HOME
        CXX = $(MINGW_HOME)/bin/g++.exe
    else ifneq (,$(wildcard C:/MinGW/bin/g++.exe))
        CXX = C:/MinGW/bin/g++.exe
    else ifneq (,$(wildcard C:/mingw64/bin/g++.exe))
        CXX = C:/mingw64/bin/g++.exe
    else ifneq (,$(wildcard C:/msys64/mingw64/bin/g++.exe))
        CXX = C:/msys64/mingw64/bin/g++.exe
    endif
endif

.PHONY: all clean static debug help

# 默认目标：静态编译
all: static

# 静态编译版本（推荐用于分发）
static:
	@echo "========== 静态编译 MiniFS =========="
	@echo "编译器: $(CXX)"
	@echo "源文件: $(SOURCES)"
	$(CXX) $(CXXFLAGS) $(STATIC_FLAGS) $(SOURCES) -o $(TARGET)
	@echo "========== 编译完成! =========="
	@echo "可执行文件: $(TARGET)"
	@echo ""

# 动态编译版本（用于开发调试）
dynamic:
	@echo "========== 动态编译 MiniFS =========="
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)
	@echo "========== 编译完成! =========="

# 调试版本
debug:
	@echo "========== 调试编译 MiniFS =========="
	$(CXX) $(CXXFLAGS) -g -DDEBUG $(SOURCES) -o $(TARGET)
	@echo "========== 编译完成! =========="

# 清理生成文件
clean:
	@echo "清理生成文件..."
ifeq ($(OS),Windows_NT)
	-del /Q $(TARGET) 2>nul
	-del /Q *.obj 2>nul
else
	rm -f $(TARGET) *.o
endif
	@echo "清理完成"

# 测试运行
test: static
	@echo "========== 运行测试 =========="
	./$(TARGET) --test

# 显示帮助信息
help:
	@echo "MiniFS 编译帮助:"
	@echo ""
	@echo "make          - 静态编译 (推荐)"
	@echo "make static   - 静态编译，适合分发"
	@echo "make dynamic  - 动态编译，用于开发"
	@echo "make debug    - 调试编译"
	@echo "make test     - 编译并运行测试"
	@echo "make clean    - 清理生成文件"
	@echo "make help     - 显示此帮助"
	@echo ""
	@echo "当前编译器: $(CXX)"
	@echo "当前系统: $(OS)"
