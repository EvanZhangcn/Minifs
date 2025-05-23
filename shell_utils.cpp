#include "fs_tests.hpp"
#include "shell_utils.hpp"
#include "minifs.hpp"

// 全局或在runInteractiveShell作用域内定义当前工作目录i-节点号
// 由于没有cd命令，它将一直为根目录
static int current_working_directory_inum = MiniFS::ROOT_INUM_CONST;
// 修改 showHelp
void showHelp() {
    std::cout << "\n========== MiniFS 文件系统命令行工具 ==========" << std::endl;
    std::cout << "可用命令:" << std::endl;
    std::cout << "  ls [路径]               - 列出指定路径的目录内容，无路径则列出当前目录" << std::endl;
    std::cout << "  mkdir <路径/新目录名>   - 创建新目录" << std::endl;
    std::cout << "  rmdir <路径/目录名>     - 删除空目录" << std::endl;
    std::cout << "  rm <路径/文件名>        - 删除文件" << std::endl;
    std::cout << "  chdir <路径>            - 切换到指定目录 (别名: cd)" << std::endl;
    std::cout << "  create <路径/新文件名>  - 创建新文件" << std::endl;
    std::cout << "  open <路径/文件名> <模式> - 打开文件，模式: r(只读), w(只写), rw(读写), c(创建)" << std::endl;
    std::cout << "  close <fd>              - 关闭文件描述符" << std::endl;
    std::cout << "  read <fd> <字节数>      - 从文件中读取指定字节数" << std::endl;
    std::cout << "  write <fd> <内容>       - 向文件中写入内容" << std::endl;
    std::cout << "  test-bitmap             - 运行位图操作测试" << std::endl;
    std::cout << "  test-directory          - 运行目录操作测试 (旧版，可能不完全兼容路径)" << std::endl;
    std::cout << "  test-file               - 运行文件操作测试" << std::endl;
    std::cout << "  format                  - 格式化文件系统" << std::endl;
    std::cout << "  save                    - 保存文件系统" << std::endl;
    std::cout << "  status                  - 显示文件系统状态" << std::endl;
    std::cout << "  help                    - 显示帮助信息" << std::endl;
    std::cout << "  exit                    - 退出程序" << std::endl;
    std::cout << "  login <用户名>          - 登录用户" << std::endl;
    std::cout << "  logout                  - 登出当前用户" << std::endl;
    std::cout << "  useradd <用户名> <UID> <GID> - 添加新用户" << std::endl;
    std::cout << "  users                   - 列出所有用户" << std::endl;
    std::cout << "  whoami                  - 显示当前用户" << std::endl;
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

// 获取当前工作目录的路径名
std::string getCwdPath(MiniFS& fs, int current_inum) {
    if (current_inum == MiniFS::ROOT_INUM_CONST) {
        return "/";
    }
    
    // 构建从当前目录到根的路径
    std::vector<std::string> path_components;
    int inum = current_inum;
    dinode node;
    
    while (inum != MiniFS::ROOT_INUM_CONST) {
        if (!fs._get_inode(inum, node)) {
            return "/错误"; // 无法获取节点信息
        }
        
        // 在父目录中查找指向当前目录的条目
        int parent_inum = fs._lookup_in_directory(inum, "..");
        if (parent_inum == MiniFS::INVALID_INUM_CONST) {
            return "/错误"; // 无法找到父目录
        }
        
        // 在父目录中查找当前目录的名称
        dinode parent_node;
        if (!fs._get_inode(parent_inum, parent_node)) {
            return "/错误"; // 无法获取父节点信息
        }
        
        // 读取父目录的内容
        int entries_count = parent_node.size / sizeof(dirent);
        dirent entries[BLOCK_SIZE / sizeof(dirent)];
        fs.readBlock(parent_node.addrs[0], entries);
        
        // 查找对应当前inum的条目
        std::string current_name;
        bool found = false;
        for (int i = 0; i < entries_count; i++) {
            if (entries[i].inum == inum && 
                std::strcmp(entries[i].name, ".") != 0 && 
                std::strcmp(entries[i].name, "..") != 0) {
                current_name = entries[i].name;
                found = true;
                break;
            }
        }
        
        if (!found) {
            return "/错误"; // 无法找到目录名
        }
        
        //一路上找到的父目录，全部放入栈中path_components
        path_components.push_back(current_name);
        inum = parent_inum; // 移动到父目录
    }
    
    // 逐个出栈：构建完整路径
    std::string full_path = "/";
    //后进先出，访问栈顶元素
    for (auto it = path_components.rbegin(); it != path_components.rend(); ++it) {
        full_path += *it + "/";
    }
    
    // 如果不是根目录，去掉末尾的'/'
    if (full_path.length() > 1) {
        full_path.pop_back();
    }
    
    return full_path;
}

// 交互式命令行界面
void runInteractiveShell(MiniFS& fs, const std::string& fsfile) {
    std::cout << "\n========== MiniFS 交互式命令行 ==========" << std::endl;
    std::cout << "当前目录 i-节点号: " << current_working_directory_inum << " (根目录)" << std::endl;
    std::cout << "输入 'help' 查看可用命令，输入 'exit' 退出" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // 定义需要用户登录的命令列表
    const std::vector<std::string> user_required_commands = {
        "mkdir", "rmdir", "rm", "cd", "chdir", "create", "open", 
        "close", "read", "write"
    };
    
    std::string input;
    while (true) {
        // 获取当前工作目录的路径并显示命令提示符
        std::string cwd_path = getCwdPath(fs, current_working_directory_inum);
        std::string user_part = fs.isLoggedIn() ? fs.getCurrentUser().getUsername() + "@" : "";
        std::cout << "\nMiniFS:" << user_part << cwd_path << "> ";
        std::getline(std::cin, input);
        
        // 去除前导和后导空格
        input.erase(0, input.find_first_not_of(" \t"));
        if (input.find_last_not_of(" \t") != std::string::npos)
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
            // 1. 系统控制命令 - 不需要登录权限检查
            if (command == "exit" || command == "quit") {
                std::cout << "正在保存并退出..." << std::endl;
                if (fs.saveFS(fsfile) == 0) {
                    std::cout << "文件系统已保存到 " << fsfile << std::endl;
                } else {
                    std::cout << "保存文件系统失败!" << std::endl;
                }
                break;
            } 
            else if (command == "help" || command == "h") {
                showHelp();
            }
            else if (command == "status") {
                showStatus(fs);
            }
            else if (command == "save") {
                if (fs.saveFS(fsfile) == 0) {
                    std::cout << "文件系统已保存到 " << fsfile << std::endl;
                } else {
                    std::cout << "保存文件系统失败!" << std::endl;
                }
            }
            else if (command == "format") {
                std::cout << "警告: 这将清除所有数据! 确认吗? (y/N): ";
                std::string confirm;
                std::getline(std::cin, confirm);
                if (confirm == "y" || confirm == "Y" || confirm == "yes") {
                    fs.format();
                    current_working_directory_inum = MiniFS::ROOT_INUM_CONST; // 重置CWD
                    std::cout << "文件系统已重新格式化. 当前目录已重置为根目录。" << std::endl;
                } else {
                    std::cout << "操作已取消" << std::endl;
                }
            }
            
            // 2. 用户管理命令 - 不需要登录权限检查
            else if (command == "login") {
                if (tokens.size() == 2) {
                    std::string username = tokens[1];
                    std::string password;
                    std::cout << "请输入密码: ";
                    std::getline(std::cin, password);
                    fs.login(username, password);
                } else {
                    std::cerr << "用法: login <用户名>" << std::endl;
                }
            }
            else if (command == "logout") {
                fs.logout();
            }
            else if (command == "useradd") {
                if (tokens.size() == 4) {
                    std::string username = tokens[1];
                    try {
                        int uid = std::stoi(tokens[2]);
                        int gid = std::stoi(tokens[3]);
                        std::string password, password_confirm;
                        std::cout << "为用户 " << username << " 设置密码: ";
                        std::getline(std::cin, password);
                        std::cout << "确认密码: ";
                        std::getline(std::cin, password_confirm);
                        
                        if (password == password_confirm) {
                            fs.addUser(username, password, uid, gid);
                        } else {
                            std::cerr << "错误: 两次输入的密码不匹配" << std::endl;
                        }
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "错误: UID 和 GID 必须是整数值" << std::endl;
                    }
                } else {
                    std::cerr << "用法: useradd <用户名> <UID> <GID>" << std::endl;
                }
            }
            else if (command == "users") {
                fs.listUsers();
            }
            else if (command == "whoami") {
                if (fs.isLoggedIn()) {
                    const User& current_user = fs.getCurrentUser();
                    std::cout << "当前用户: " << current_user.getUsername() 
                              << " (UID: " << current_user.getUid() 
                              << ", GID: " << current_user.getGid() << ")" << std::endl;
                } else {
                    std::cout << "未登录" << std::endl;
                }
            }
            
            // 3. 测试命令 - 不需要登录权限检查
            else if (command == "test-bitmap") {
                test_bitmap_operations(fs);
            } 
            else if (command == "test-directory") {
                std::cout << "注意: test-directory 使用旧的基于inum的mkdir，可能不完全反映路径行为。" << std::endl;
                test_directory_operations(fs);
            } 
            else if (command == "test-file") {
                std::cout << "开始执行文件操作测试..." << std::endl;
                test_file_operations(fs);
            }
            else if (command == "test-user") {
                std::cout << "开始执行用户管理功能测试..." << std::endl;
                test_user_operations(fs);
            }
            
            // 4. 文件系统命令 - 需要登录权限检查
            else {
                // 对于需要用户登录的操作，进行权限检查
                bool requires_login = std::find(user_required_commands.begin(), 
                                              user_required_commands.end(), 
                                              command) != user_required_commands.end();
                
                if (requires_login && !fs.isLoggedIn()) {
                    std::cerr << "错误: 命令 '" << command << "' 需要用户登录" << std::endl;
                    std::cerr << "请使用 'login <用户名>' 登录系统" << std::endl;
                    continue;
                }
                
                // 4.1 目录浏览与查看命令
                if (command == "ls") {
                    if (tokens.size() == 1) {
                        // ls (列出当前工作目录)
                        std::cout << "列出当前目录 (inum " << current_working_directory_inum << ") 内容:" << std::endl;
                        fs.listDir(current_working_directory_inum);
                    } else if (tokens.size() == 2) {
                        // ls <路径>
                        std::string path_arg = tokens[1];
                        int target_inum = fs.resolve_path_to_inum(path_arg, current_working_directory_inum);
                        
                        if (target_inum != MiniFS::INVALID_INUM_CONST) {
                            fs.listDir(target_inum);
                        } else {
                            std::cerr << "错误: 跄径 '" << path_arg << "' 解析失败或不存在。" << std::endl;
                        }
                    } else {
                        std::cerr << "用法: ls [路径]" << std::endl;
                    }
                }
                
                // 4.2 目录操作命令
                else if (command == "mkdir") {
                    if (tokens.size() == 2) {
                        // 规范化路径参数
                        std::string full_path_arg = tokens[1];
                        std::string parent_path_str;
                        std::string new_dir_name_str;
                        
                        // 解析路径，分离父目录和新目录名
                        if (!parsePath(full_path_arg, parent_path_str, new_dir_name_str)) {
                            continue;
                        }
                        
                        // 检查目录名是否有效
                        if (!isValidName(new_dir_name_str, full_path_arg, true)) {
                            continue;
                        }
                        
                        // 解析父目录inum
                        int parent_dir_inum = resolveParentPath(fs, parent_path_str);
                        if (parent_dir_inum == MiniFS::INVALID_INUM_CONST) {
                            std::cerr << "错误: 父路径 '" << parent_path_str << "' 解析失败或不存在。" << std::endl;
                            continue;
                        }
                        
                        // 创建目录
                        int result = fs.mkdir(parent_dir_inum, new_dir_name_str.c_str());
                        if (result != MiniFS::INVALID_INUM_CONST) {
                            std::cout << "成功创建目录 '" << full_path_arg << "' (i-节点号: " << result << ")." << std::endl;
                        }
                    } else {
                        std::cerr << "用法: mkdir <路径/新目录名>" << std::endl;
                    }
                }
                else if (command == "rmdir") {
                    if (tokens.size() == 2) {
                        // rmdir <路径/目录名>
                        std::string full_path_arg = tokens[1];
                        std::string parent_path_str;
                        std::string dir_name_str;
                        
                        // 规范化路径
                        std::string temp_full_path = normalizePath(full_path_arg);
                        
                        // 特殊情况：禁止删除根目录
                        if (temp_full_path == "/") {
                            std::cerr << "错误: 不能删除根目录。" << std::endl;
                            continue;
                        }
                        
                        // 解析路径，分离父目录和目录名
                        if (!parsePath(temp_full_path, parent_path_str, dir_name_str)) {
                            continue;
                        }
                        
                        // 检查目录名是否有效
                        if (!isValidName(dir_name_str, full_path_arg, true)) {
                            continue;
                        }
                        
                        // 解析父目录inum
                        int parent_dir_inum = resolveParentPath(fs, parent_path_str);
                        if (parent_dir_inum == MiniFS::INVALID_INUM_CONST) {
                            std::cerr << "错误: 父路径 '" << parent_path_str << "' 解析失败或不存在。" << std::endl;
                            continue;
                        }
                        
                        // 删除目录
                        int result = fs.rmdir(parent_dir_inum, dir_name_str.c_str());
                        if (result == 0) {
                            std::cout << "成功删除目录: '" << full_path_arg << "'" << std::endl;
                        }
                    } else {
                        std::cerr << "用法: rmdir <路径/目录名>" << std::endl;
                    }
                }
                else if (command == "chdir" || command == "cd") {
                    if (tokens.size() == 2) {
                        std::string target_path = normalizePath(tokens[1]);
                        
                        if (target_path.empty()) {
                            std::cerr << "错误: 路径不能为空。" << std::endl;
                            continue;
                        }
                        
                        // 解析目标路径
                        int target_inum = fs.resolve_path_to_inum(target_path, current_working_directory_inum);
                        
                        if (target_inum != MiniFS::INVALID_INUM_CONST) {
                            // 验证目标是否为目录
                            dinode target_node;
                            if (fs._get_inode(target_inum, target_node)) {
                                if (target_node.type == T_DIR) {
                                    current_working_directory_inum = target_inum;
                                    std::cout << "成功切换到目录: " << target_path << " (i-节点号: " << target_inum << ")" << std::endl;
                                } else {
                                    std::cerr << "错误: '" << target_path << "' 不是一个目录。" << std::endl;
                                }
                            } else {
                                std::cerr << "错误: 无法读取目标路径的i-节点信息。" << std::endl;
                            }
                        } else {
                            std::cerr << "错误: 路径 '" << target_path << "' 解析失败或不存在。" << std::endl;
                        }
                    } else {
                        std::cerr << "用法: chdir <路径> 或 cd <路径>" << std::endl;
                    }
                }
                
                // 4.3 文件操作命令
                else if (command == "create") {
                    if (tokens.size() == 2) {
                        std::string full_path_arg = tokens[1];
                        std::string parent_path_str;
                        std::string new_file_name_str;
                        
                        // 解析路径，分离父目录和文件名
                        if (!parsePath(full_path_arg, parent_path_str, new_file_name_str)) {
                            continue;
                        }
                        
                        // 检查文件名是否有效
                        if (!isValidName(new_file_name_str, full_path_arg, false)) {
                            continue;
                        }
                        
                        // 解析父目录inum
                        int parent_dir_inum = resolveParentPath(fs, parent_path_str);
                        if (parent_dir_inum == MiniFS::INVALID_INUM_CONST) {
                            std::cerr << "错误: 父路径 '" << parent_path_str << "' 解析失败或不存在。" << std::endl;
                            continue;
                        }
                        
                        // 创建文件
                        int result = fs.create(parent_dir_inum, new_file_name_str.c_str());
                        if (result != MiniFS::INVALID_INUM_CONST) {
                            std::cout << "成功创建文件: '" << full_path_arg << "' (i-节点号: " << result << ")." << std::endl;
                        }
                    } else {
                        std::cerr << "用法: create <路径/新文件名>" << std::endl;
                    }
                }
                else if (command == "rm") {
                    if (tokens.size() == 2) {
                        std::string full_path_arg = tokens[1];
                        std::string parent_path_str;
                        std::string file_name_str;
                        
                        // 解析路径，分离父目录和文件名
                        if (!parsePath(full_path_arg, parent_path_str, file_name_str)) {
                            continue;
                        }
                        
                        // 检查文件名是否有效
                        if (!isValidName(file_name_str, full_path_arg, false)) {
                            continue;
                        }
                        
                        // 解析父目录inum
                        int parent_dir_inum = resolveParentPath(fs, parent_path_str);
                        if (parent_dir_inum == MiniFS::INVALID_INUM_CONST) {
                            std::cerr << "错误: 父路径 '" << parent_path_str << "' 解析失败或不存在。" << std::endl;
                            continue;
                        }
                        
                        // 删除文件
                        int result = fs.rm(parent_dir_inum, file_name_str.c_str());
                        if (result == 0) {
                            std::cout << "成功删除文件: '" << full_path_arg << "'" << std::endl;
                        }
                    } else {
                        std::cerr << "用法: rm <路径/文件名>" << std::endl;
                    }
                }
                
                // 4.4 文件读写命令
                else if (command == "open") {
                    if (tokens.size() == 3) {
                        std::string full_path_arg = tokens[1];
                        std::string mode_str = tokens[2];
                        std::string parent_path_str;
                        std::string file_name_str;
                        int flags = 0;
                        
                        // 解析打开模式
                        if (mode_str == "r") {
                            flags = MiniFS::O_RDONLY;
                        } else if (mode_str == "w") {
                            flags = MiniFS::O_WRONLY;
                        } else if (mode_str == "rw") {
                            flags = MiniFS::O_RDWR;
                        } else if (mode_str == "c") {
                            flags = MiniFS::O_CREATE | MiniFS::O_RDWR;
                        } else {
                            std::cerr << "错误: 无效的打开模式 '" << mode_str << "'" << std::endl;
                            std::cerr << "有效模式: r(只读), w(只写), rw(读写), c(创建)" << std::endl;
                            continue;
                        }
                        
                        // 解析路径，分离父目录和文件名
                        if (!parsePath(full_path_arg, parent_path_str, file_name_str)) {
                            continue;
                        }
                        
                        // 检查文件名是否有效
                        if (!isValidName(file_name_str, full_path_arg, false)) {
                            continue;
                        }
                        
                        // 解析父目录inum
                        int parent_dir_inum = resolveParentPath(fs, parent_path_str);
                        if (parent_dir_inum == MiniFS::INVALID_INUM_CONST) {
                            std::cerr << "错误: 父路径 '" << parent_path_str << "' 解析失败或不存在。" << std::endl;
                            continue;
                        }
                        
                        // 打开文件
                        int fd = fs.open(parent_dir_inum, file_name_str.c_str(), flags);
                        if (fd != -1) {
                            std::cout << "成功打开文件: '" << full_path_arg << "' (文件描述符: " << fd << ")." << std::endl;
                        }
                    } else {
                        std::cerr << "用法: open <路径/文件名> <模式>" << std::endl;
                        std::cerr << "  模式: r(只读), w(只写), rw(读写), c(创建)" << std::endl;
                    }
                }
                else if (command == "close") {
                    if (tokens.size() == 2) {
                        try {
                            int fd = std::stoi(tokens[1]);
                            int result = fs.close(fd);
                            if (result == 0) {
                                std::cout << "成功关闭文件描述符: " << fd << std::endl;
                            }
                        } catch (const std::invalid_argument& e) {
                            std::cerr << "错误: 无效的文件描述符，必须是一个数字" << std::endl;
                        } catch (const std::out_of_range& e) {
                            std::cerr << "错误: 文件描述符超出范围" << std::endl;
                        }
                    } else {
                        std::cerr << "用法: close <文件描述符>" << std::endl;
                    }
                }
                else if (command == "read") {
                    if (tokens.size() == 3) {
                        try {
                            int fd = std::stoi(tokens[1]);
                            int count = std::stoi(tokens[2]);
                            
                            if (count <= 0) {
                                std::cerr << "错误: 读取字节数必须大于0" << std::endl;
                                continue;
                            }
                            
                            if (count > 1024) {
                                std::cout << "警告: 读取字节数较大，已限制为1024字节" << std::endl;
                                count = 1024;
                            }
                            
                            // 分配缓冲区来存储读取的数据
                            std::unique_ptr<char[]> buffer(new char[count + 1]);
                            if (!buffer) {
                                std::cerr << "错误: 内存分配失败" << std::endl;
                                continue;
                            }
                            
                            // 读取数据
                            int bytes_read = fs.read(fd, buffer.get(), count);
                            
                            if (bytes_read > 0) {
                                // 确保字符串正确结尾
                                buffer[bytes_read] = '\0';
                                
                                std::cout << "从文件描述符 " << fd << " 读取了 " << bytes_read << " 字节:" << std::endl;
                                std::cout << "----开始内容----" << std::endl;
                                
                                // 检查并打印读取的内容
                                displayReadContent(buffer.get(), bytes_read);
                                
                                std::cout << "----结束内容----" << std::endl;
                            } else if (bytes_read == 0) {
                                std::cout << "已到达文件末尾，未读取任何字节" << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "错误: " << e.what() << std::endl;
                        }
                    } else {
                        std::cerr << "用法: read <文件描述符> <字节数>" << std::endl;
                    }
                }
                else if (command == "write") {
                    if (tokens.size() >= 3) {
                        try {
                            int fd = std::stoi(tokens[1]);
                            
                            // 获取要写入的内容
                            std::string content;
                            if (tokens[2] == "$") {
                                std::cout << "请输入要写入的内容（以单独的'.'行结束）：" << std::endl;
                                std::string line;
                                while (std::getline(std::cin, line) && line != ".") {
                                    content += line + "\n";
                                }
                            } else {
                                for (size_t i = 2; i < tokens.size(); ++i) {
                                    content += tokens[i];
                                    if (i < tokens.size() - 1) {
                                        content += " ";
                                    }
                                }
                            }
                            
                            // 执行写入操作
                            int bytes_written = fs.write(fd, content.c_str(), content.length());
                            
                            if (bytes_written > 0) {
                                std::cout << "成功向文件描述符 " << fd << " 写入 " << bytes_written << " 字节" << std::endl;
                            }
                        } catch (const std::invalid_argument& e) {
                            std::cerr << "错误: 无效的文件描述符，必须是一个数字" << std::endl;
                        } catch (const std::out_of_range& e) {
                            std::cerr << "错误: 文件描述符超出范围" << std::endl;
                        }
                    } else {
                        std::cerr << "用法: write <文件描述符> <内容>" << std::endl;
                        std::cerr << "  或: write <文件描述符> $ (然后逐行输入内容，以单独的'.'行结束)" << std::endl;
                    }
                }
                else {
                    std::cerr << "未知命令: " << command << std::endl;
                    std::cerr << "输入 'help' 查看可用命令" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "执行命令时发生错误: " << e.what() << std::endl;
        }
    }
}

// 辅助函数：规范化路径（移除多余的斜杠）
std::string normalizePath(const std::string& path) {
    std::string result = path;
    
    // 移除末尾的 '/' (除非路径就是 "/")
    while (result.length() > 1 && result.back() == '/') {
        result.pop_back();
    }
    
    // 处理 "///" 类型路径
    if (result.empty() && !path.empty() && path[0] == '/') {
        result = "/";
    }
    
    return result;
}

// 辅助函数：将路径解析为父路径和名称部分
bool parsePath(const std::string& full_path, std::string& parent_path, std::string& name) {
    std::string temp_path = normalizePath(full_path);
    
    if (temp_path.empty()) {
        std::cerr << "错误: 路径不能为空。" << std::endl;
        return false;
    }
    
    size_t last_slash_pos = temp_path.find_last_of('/');
    
    if (last_slash_pos == std::string::npos) { // 没有斜杠, e.g., "name"
        parent_path = ""; // 表示当前工作目录
        name = temp_path;
    } else if (last_slash_pos == 0) { // 斜杠在开头, e.g., "/name"
        parent_path = "/";
        name = temp_path.substr(1);
    } else { // 有路径分隔, e.g., "path/to/name" or "/path/to/name"
        parent_path = temp_path.substr(0, last_slash_pos);
        name = temp_path.substr(last_slash_pos + 1);
    }
    
    return true;
}

// 辅助函数：检查名称是否有效
bool isValidName(const std::string& name, const std::string& full_path, bool is_dir) {
    const char* type = is_dir ? "目录" : "文件";
    
    if (name.empty() || name == "." || name == "..") {
        std::cerr << "错误: 无效的" << type << "名 '" << name << "' (来自路径: '" << full_path << "')." << std::endl;
        return false;
    }
    
    if (name.length() >= DIRSIZ) {
        std::cerr << "错误: " << type << "名称 '" << name << "' 过长 (最大 " << DIRSIZ-1 << " 字符)." << std::endl;
        return false;
    }
    
    return true;
}

// 辅助函数：解析父路径到i-节点号
int resolveParentPath(MiniFS& fs, const std::string& parent_path) {
    if (parent_path.empty()) { // 在当前工作目录下
        return current_working_directory_inum;
    } else { // 解析指定路径
        return fs.resolve_path_to_inum(parent_path, current_working_directory_inum);
    }
}

// 辅助函数：显示读取的内容
void displayReadContent(const char* buffer, int bytes_read) {
    // 检查内容是否都是可打印字符
    bool isPrintable = true;
    for (int i = 0; i < bytes_read; i++) {
        if (!isprint(static_cast<unsigned char>(buffer[i])) && 
            buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '\t') {
            isPrintable = false;
            break;
        }
    }
    
    if (isPrintable) {
        std::cout << buffer << std::endl;
    } else {
        std::cout << "【包含二进制数据，以十六进制显示】" << std::endl;
        for (int i = 0; i < bytes_read; i++) {
            printf("%02X ", static_cast<unsigned char>(buffer[i]));
            if ((i + 1) % 16 == 0) std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}