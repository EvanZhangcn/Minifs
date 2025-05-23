#include "minifs.hpp"

void test_bitmap_operations(MiniFS& fs) 
{
    std::cout << "--- 开始位图操作测试 ---" << std::endl;

    // 测试 inode 位图
    int inode_bm_start = INODE_BITMAP_BLOCK_START; // 假设你知道这些常量
    int total_inodes = INODE_NUM;

    // 1. 测试 find_free_bit on empty inode bitmap
    int free_inode = fs.find_free_bit(inode_bm_start, total_inodes, 1); 
    std::cout << "空i-节点位图，找到的第一个空闲i-节点: " << free_inode << (free_inode == 2 ? " (预期)" : " (异常!)") << std::endl;

    // 2. 测试 set_bit 和 test_bit for inode
    if (free_inode != -1) {
        fs.set_bit(inode_bm_start, free_inode);
        std::cout << "设置i-节点 " << free_inode << " 为已用." << std::endl;
        bool is_set = fs.test_bit(inode_bm_start, free_inode);
        std::cout << "测试i-节点 " << free_inode << " 是否已用: " << (is_set ? "是" : "否") << (is_set ? " (预期)" : " (异常!)") << std::endl;
    }

    // 3. 再次查找空闲inode
    int next_free_inode = fs.find_free_bit(inode_bm_start, total_inodes, 1);
    std::cout << "再次查找空闲i-节点: " << next_free_inode << (next_free_inode == (free_inode != -1 ? free_inode + 1 : 0) ? " (预期)" : " (异常!)") << std::endl;

    // 4. 测试 clear_bit for inode
    if (free_inode != -1) {
        fs.clear_bit(inode_bm_start, free_inode);
        std::cout << "清除i-节点 " << free_inode << " 为空闲." << std::endl;
        bool is_set_after_clear = fs.test_bit(inode_bm_start, free_inode);
        std::cout << "测试i-节点 " << free_inode << " 是否已用 (清除后): " << (is_set_after_clear ? "是" : "否") << (!is_set_after_clear ? " (预期)" : " (异常!)") << std::endl;
    }

    // ... 类似地为数据块位图添加测试 ...
    std::cout << "--- 位图操作测试结束 ---" << std::endl;
}

void test_directory_operations(MiniFS& fs) {
    std::cout << "\n--- 开始目录操作测试 ---" << std::endl;
    
    // 1. 查看根目录内容（初始应该只有 . 和 ..）
    std::cout << "初始根目录内容:" << std::endl;
    fs.listRoot();
      // 2. 在根目录下创建一个新目录
    std::cout << "\n创建目录 'new_dir':" << std::endl;
    int root_inum = 1; // 假设根目录的i-节点号为1
    int test_dir_inum = fs.mkdir(root_inum, "new_dir");
    
    if (test_dir_inum != -1) {
        // 3. 再次查看根目录内容（现在应该包含 "test_dir"）
        std::cout << "\n创建目录后的根目录内容:" << std::endl;
        fs.listRoot();
        
        // 4. 查看新创建的目录内容（应该有 . 和 ..）
        std::cout << "\n新创建目录的内容:" << std::endl;
        fs.listDir(test_dir_inum);
        
        // 5. 在新目录下创建子目录
        std::cout << "\n在 'test_dir' 下创建子目录 'sub_dir':" << std::endl;
        int sub_dir_inum = fs.mkdir(test_dir_inum, "sub_dir");
        
        if (sub_dir_inum != -1) {
            // 6. 查看 test_dir 目录内容（现在应该包含 "sub_dir"）
            std::cout << "\n创建子目录后的 'test_dir' 内容:" << std::endl;
            fs.listDir(test_dir_inum);
            
            // 7. 查看子目录内容
            std::cout << "\n子目录 'sub_dir' 的内容:" << std::endl;
            fs.listDir(sub_dir_inum);
        }
    }
    
    std::cout << "--- 目录操作测试结束 ---\n" << std::endl;
}

// 全局或在runInteractiveShell作用域内定义当前工作目录i-节点号
// 由于没有cd命令，它将一直为根目录
static int current_working_directory_inum = MiniFS::ROOT_INUM_CONST;
// 修改 showHelp
void showHelp() {
    std::cout << "\n========== MiniFS 文件系统命令行工具 ==========" << std::endl;
    std::cout << "可用命令:" << std::endl;
    std::cout << "  ls [路径]               - 列出指定路径的目录内容，无路径则列出当前目录" << std::endl;
    std::cout << "  mkdir <路径/新目录名>   - 创建新目录" << std::endl;
    std::cout << "  test-bitmap             - 运行位图操作测试" << std::endl;
    std::cout << "  test-directory          - 运行目录操作测试 (旧版，可能不完全兼容路径)" << std::endl;
    std::cout << "  format                  - 格式化文件系统" << std::endl;
    std::cout << "  save                    - 保存文件系统" << std::endl;
    std::cout << "  status                  - 显示文件系统状态" << std::endl;
    std::cout << "  help                    - 显示帮助信息" << std::endl;
    std::cout << "  exit                    - 退出程序" << std::endl;
    std::cout << "===============================================" << std::endl;
}


// 显示文件系统状态
void showStatus(MiniFS& fs) {
    std::cout << "\n========== 文件系统状态 ==========" << std::endl;
    std::cout << "总块数: " << BLOCK_COUNT << std::endl;
    std::cout << "块大小: " << BLOCK_SIZE << " 字节" << std::endl;
    std::cout << "i-节点总数: " << INODE_NUM << std::endl;
    std::cout << "数据块总数: " << DATA_BLOCKS_NUM << std::endl;
    std::cout << "i-节点区起始块: " << INODE_START << std::endl;
    std::cout << "数据区起始块: " << DATA_START << std::endl;
    std::cout << "=================================" << std::endl;
}

// 解析命令行输入,按空格分割
std::vector<std::string> parseCommand(const std::string& input) {
    std::vector<std::string> tokens; //tokens栈，存储
    //用字符串流，将命令行输入按空格进行分割
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// 交互式命令行界面
void runInteractiveShell(MiniFS& fs, const std::string& fsfile) {
    std::cout << "\n========== MiniFS 交互式命令行 ==========" << std::endl;
    std::cout << "当前目录 i-节点号: " << current_working_directory_inum << " (根目录)" << std::endl;
    std::cout << "输入 'help' 查看可用命令，输入 'exit' 退出" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    std::string input;
    while (true) {
        std::cout << "\nMiniFS> "; // 可以考虑显示当前路径，但需要更复杂的 CWD 管理
        std::getline(std::cin, input);
        
        //去除前导和后导空格
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);
        
        if (input.empty()) {
            continue;
        }
        
        std::vector<std::string> tokens = parseCommand(input);
        if (tokens.empty()) {
            continue;
        }
        
        std::string command = tokens[0];
        try {
            if (command == "exit" || command == "quit") 
            {
                // ... (保持不变)
                std::cout << "正在保存并退出..." << std::endl;
                if (fs.saveFS(fsfile) == 0) {
                    std::cout << "文件系统已保存到 " << fsfile << std::endl;
                } 
                else {
                    std::cout << "保存文件系统失败!" << std::endl;
                }
                break;
            } 
            else if (command == "help" || command == "h") 
            {
                showHelp();
            } 
            else if (command == "ls") 
            {
                if (tokens.size() == 1) 
                {
                    // ls (列出当前工作目录)
                    std::cout << "列出当前目录 (inum " << current_working_directory_inum << ") 内容:" << std::endl;
                    fs.listDir(current_working_directory_inum);
                } 
                else if (tokens.size() == 2) 
                {
                    // ls <路径>
                    std::string path_arg = tokens[1];
                    int target_inum = fs.resolve_path_to_inum(path_arg, current_working_directory_inum);
                    
                    if (target_inum != MiniFS::INVALID_INUM_CONST) {
                        // listDir 内部会检查是否为目录类型
                        fs.listDir(target_inum);
                    } else {
                        std::cerr << "错误: 路径 '" << path_arg << "' 解析失败或不存在。" << std::endl;
                    }
                } 
                else 
                {
                    std::cerr << "用法: ls [路径]" << std::endl;
                }
            } 
            else if (command == "mkdir") 
            {
                if (tokens.size() == 2) {
                    // mkdir <路径/新目录名>
                    std::string full_path_arg = tokens[1];
                    std::string parent_path_str;
                    std::string new_dir_name_str;

                    // 规范化：移除末尾的 '/' (除非路径就是 "/")
                    std::string temp_full_path = full_path_arg;
                    while(temp_full_path.length() > 1 && temp_full_path.back() == '/') {
                        temp_full_path.pop_back();
                    }
                     if (temp_full_path.empty() && !full_path_arg.empty() && full_path_arg[0] == '/') {
                        temp_full_path = "/"; // 处理 "///" 类型路径
                    }

                    if (temp_full_path.empty()){
                        std::cerr << "错误: 路径不能为空。" << std::endl;
                        continue;
                    }

                    size_t last_slash_pos = temp_full_path.find_last_of('/');

                    if (last_slash_pos == std::string::npos) { // 没有斜杠, e.g., "newdir"
                        parent_path_str = ""; // 表示当前工作目录
                        new_dir_name_str = temp_full_path;
                    } else if (last_slash_pos == 0) { // 斜杠在开头, e.g., "/newdir"
                        parent_path_str = "/";
                        new_dir_name_str = temp_full_path.substr(1);
                    } else { // 有路径分隔, e.g., "path/to/newdir" or "/path/to/newdir"
                        parent_path_str = temp_full_path.substr(0, last_slash_pos);
                        new_dir_name_str = temp_full_path.substr(last_slash_pos + 1);
                    }

                    if (new_dir_name_str.empty() || new_dir_name_str == "." || new_dir_name_str == "..") {
                        std::cerr << "错误: 无效的新目录名 '" << new_dir_name_str << "' (来自路径: '" << full_path_arg << "')." << std::endl;
                        continue;
                    }
                    if (new_dir_name_str.length() >= DIRSIZ) {
                         std::cerr << "错误: 目录名称 '" << new_dir_name_str << "' 过长 (最大 " << DIRSIZ-1 << " 字符)." << std::endl;
                         continue;
                     }


                    int parent_dir_inum;
                    if (parent_path_str.empty()) { // mkdir newdir (在CWD下)
                        parent_dir_inum = current_working_directory_inum;
                    } else { // mkdir path/newdir or /path/newdir
                        parent_dir_inum = fs.resolve_path_to_inum(parent_path_str, current_working_directory_inum);
                    }

                    if (parent_dir_inum != MiniFS::INVALID_INUM_CONST) {
                        int result = fs.mkdir(parent_dir_inum, new_dir_name_str.c_str());
                        if (result != MiniFS::INVALID_INUM_CONST) {
                            std::cout << "成功创建目录 '" << full_path_arg << "' (i-节点号: " << result << ")." << std::endl;
                        } else {
                            // fs.mkdir 内部应该会打印详细错误
                            // std::cerr << "创建目录 '" << full_path_arg << "' 失败。" << std::endl;
                        }
                    } else {
                        std::cerr << "错误: 父路径 '" << parent_path_str << "' 解析失败或不存在。" << std::endl;
                    }
                } else {
                    std::cerr << "用法: mkdir <路径/新目录名>" << std::endl;
                }
            }
            else if (command == "test-bitmap") { // 保留旧的测试命令
                test_bitmap_operations(fs);
            } 
            else if (command == "test-directory") { // 保留旧的测试命令
                std::cout << "注意: test-directory 使用旧的基于inum的mkdir，可能不完全反映路径行为。" << std::endl;
                test_directory_operations(fs);
            } 
            else if (command == "format") 
            {
                std::cout << "警告: 这将清除所有数据! 确认吗? (y/N): ";
                std::string confirm;
                std::getline(std::cin, confirm);
                if (confirm == "y" || confirm == "Y" || confirm == "yes") {
                    fs.format();
                    current_working_directory_inum = MiniFS::ROOT_INUM_CONST; // 重置CWD
                    std::cout << "文件系统已重新格式化. 当前目录已重置为根目录。" << std::endl;
                } 
                else {
                    std::cout << "操作已取消" << std::endl;
                }
            } 
            else if (command == "save") 
            {
                 if (fs.saveFS(fsfile) == 0) {
                    std::cout << "文件系统已保存到 " << fsfile << std::endl;
                } else {
                    std::cout << "保存文件系统失败!" << std::endl;
                }
            } 
            else if (command == "status") 
            {
                showStatus(fs);
            } 
            else {
                std::cerr << "未知命令: " << command << std::endl;
                std::cerr << "输入 'help' 查看可用命令" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "执行命令时发生错误: " << e.what() << std::endl;
        }
    }
}

// 在 main 函数中
int main(int argc, char *argv[]) {
    const std::string fsfile = "my_unix_fs.dat";
    MiniFS fs;
    
    std::cout << "========== MiniFS 文件系统启动 ==========" << std::endl;
    
    // 尝试加载已有文件系统，如果失败则格式化
    std::cout << "尝试加载文件系统镜像: " << fsfile << std::endl;
    MiniFS::FSStatus load_result = fs.loadFS(fsfile);
    
    if (load_result != MiniFS::FSStatus::OK) {
        std::cout << "未检测到有效的文件系统镜像，正在格式化..." << std::endl;
        fs.format();
        std::cout << "文件系统格式化完成!" << std::endl;
    } else {
        std::cout << "文件系统镜像加载成功!" << std::endl;
    }
    
    // 检查是否有命令行参数来决定运行模式
    if (argc > 1 && std::string(argv[1]) == "--test") {
        // 测试模式：运行所有测试
        std::cout << "\n========== 测试模式 ==========" << std::endl;
        test_bitmap_operations(fs);
        test_directory_operations(fs);
        
        // 保存文件系统状态
        std::cout << "正在保存文件系统..." << std::endl;
        if (fs.saveFS(fsfile) == 0) {
            std::cout << "文件系统已成功保存到 " << fsfile << std::endl;
        } else {
            std::cout << "保存文件系统失败!" << std::endl;
        }
    } 
    else 
    {
        // 交互模式：启动命令行界面
        runInteractiveShell(fs, fsfile);
    }
    
    std::cout << "程序结束" << std::endl;
    return 0;
}