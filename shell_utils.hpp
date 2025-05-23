#ifndef FS_UTILS_HPP
#define FS_UTILS_HPP

#include "fs_tests.hpp"
#include "minifs.hpp"
#include <algorithm> // 添加 std::find 的头文件

// 原有函数声明
void showHelp();
void showStatus(MiniFS& fs); // 改回非const以适应当前实现
std::vector<std::string> parseCommand(const std::string& input);
void runInteractiveShell(MiniFS& fs, const std::string& fsfile);
std::string getCwdPath(MiniFS& fs, int current_inum);

// 添加辅助函数的声明
std::string normalizePath(const std::string& path);
bool parsePath(const std::string& full_path, std::string& parent_path, std::string& name);
bool isValidName(const std::string& name, const std::string& full_path, bool is_dir);
int resolveParentPath(MiniFS& fs, const std::string& parent_path);
void displayReadContent(const char* buffer, int bytes_read);

#endif // FS_UTILS_HPP