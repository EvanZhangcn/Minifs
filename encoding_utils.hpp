#ifndef ENCODING_UTILS_HPP
#define ENCODING_UTILS_HPP

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#include <iostream>

class EncodingUtils {
public:
    // 初始化控制台编码，解决中文乱码问题
    static bool initConsoleEncoding() {
#ifdef _WIN32
        // 设置控制台输入输出编码为UTF-8
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
        
        // 启用ANSI转义序列支持
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
        
        return true;
#else
        // Linux/macOS 通常默认UTF-8
        return true;
#endif
    }
};

#endif // ENCODING_UTILS_HPP
