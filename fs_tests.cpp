#include "fs_tests.hpp"
#include "shell_utils.hpp"
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

void test_file_operations(MiniFS& fs) {
    std::cout << "\n--- 开始文件操作测试 ---" << std::endl;
    
    // 1. 创建测试目录
    int root_inum = MiniFS::ROOT_INUM_CONST;
    std::cout << "\n步骤1: 创建测试目录 'test_files'" << std::endl;
    int test_dir_inum = fs.mkdir(root_inum, "test_files");
    
    if (test_dir_inum != MiniFS::INVALID_INUM_CONST) {
        // 2. 创建测试文件
        std::cout << "\n步骤2: 在测试目录下创建文件 'hello.txt'" << std::endl;
        int file_inum = fs.create(test_dir_inum, "hello.txt");
        
        if (file_inum != MiniFS::INVALID_INUM_CONST) {
            std::cout << "文件创建成功，i-节点号: " << file_inum << std::endl;
            
            // 3. 打开文件进行写入
            std::cout << "\n步骤3: 以写入模式打开文件" << std::endl;
            int fd = fs.open(test_dir_inum, "hello.txt", MiniFS::O_WRONLY);
            
            if (fd != -1) {
                std::cout << "文件打开成功，文件描述符: " << fd << std::endl;
                
                // 4. 写入文件
                std::cout << "\n步骤4: 向文件写入数据" << std::endl;
                const char* test_data = "Hello, MiniFS!\nThis is a test file.\n";
                int write_result = fs.write(fd, test_data, strlen(test_data));
                
                if (write_result > 0) {
                    std::cout << "成功写入 " << write_result << " 字节" << std::endl;
                    
                    // 5. 关闭文件
                    std::cout << "\n步骤5: 关闭文件" << std::endl;
                    if (fs.close(fd) == 0) {
                        std::cout << "文件成功关闭" << std::endl;
                        
                        // 6. 重新打开文件以读取
                        std::cout << "\n步骤6: 以读取模式重新打开文件" << std::endl;
                        fd = fs.open(test_dir_inum, "hello.txt", MiniFS::O_RDONLY);
                        
                        if (fd != -1) {
                            std::cout << "文件打开成功，文件描述符: " << fd << std::endl;
                            
                            // 7. 读取文件内容
                            std::cout << "\n步骤7: 读取文件内容" << std::endl;
                            char buffer[100];
                            int read_result = fs.read(fd, buffer, sizeof(buffer) - 1);
                            
                            if (read_result > 0) {
                                buffer[read_result] = '\0';  // 确保字符串正确终止
                                std::cout << "成功读取 " << read_result << " 字节" << std::endl;
                                std::cout << "文件内容: \n" << buffer << std::endl;
                                
                                // 检查内容是否匹配
                                if (strcmp(buffer, test_data) == 0) {
                                    std::cout << "读取的内容与写入的内容匹配 ✓" << std::endl;
                                } else {
                                    std::cout << "警告: 读取的内容与写入的内容不匹配 ✗" << std::endl;
                                }
                            } else {
                                std::cout << "读取文件失败，返回值: " << read_result << std::endl;
                            }
                            
                            // 8. 关闭文件
                            std::cout << "\n步骤8: 再次关闭文件" << std::endl;
                            if (fs.close(fd) == 0) {
                                std::cout << "文件成功关闭" << std::endl;
                            } else {
                                std::cout << "关闭文件失败" << std::endl;
                            }
                        } else {
                            std::cout << "以读取模式打开文件失败" << std::endl;
                        }
                    } else {
                        std::cout << "关闭文件失败" << std::endl;
                    }
                } else {
                    std::cout << "写入文件失败，返回值: " << write_result << std::endl;
                }
            } else {
                std::cout << "打开文件失败" << std::endl;
            }
            
            // 9. 追加测试：创建另一个文件并进行读写测试
            std::cout << "\n步骤9: 创建另一个文件进行大数据读写测试" << std::endl;
            int file2_inum = fs.create(test_dir_inum, "bigfile.dat");
            
            if (file2_inum != MiniFS::INVALID_INUM_CONST) {
                std::cout << "第二个文件创建成功，i-节点号: " << file2_inum << std::endl;
                
                // 10. 打开文件进行写入
                fd = fs.open(test_dir_inum, "bigfile.dat", MiniFS::O_WRONLY);
                
                if (fd != -1) {
                    std::cout << "\n步骤10: 写入大量数据（尝试跨越多个数据块）" << std::endl;
                    
                    // 创建一个大数据块
                    std::string large_data;
                    for (int i = 0; i < 10; i++) {  // 创建足够大的数据以跨越多个块
                        char line[64];
                        snprintf(line, sizeof(line), "Line %d: This is test data for MiniFS large file test.\n", i);
                        large_data += line;
                    }
                    
                    int big_write = fs.write(fd, large_data.c_str(), large_data.length());
                    if (big_write > 0) {
                        std::cout << "成功写入 " << big_write << " 字节数据" << std::endl;
                        fs.close(fd);
                        
                        // 11. 尝试重新打开并读取部分数据
                        fd = fs.open(test_dir_inum, "bigfile.dat", MiniFS::O_RDONLY);
                        if (fd != -1) {
                            std::cout << "\n步骤11: 读取部分数据（前100字节）" << std::endl;
                            char small_buffer[101] = {0};
                            int partial_read = fs.read(fd, small_buffer, 100);
                            
                            if (partial_read > 0) {
                                small_buffer[partial_read] = '\0';
                                std::cout << "成功读取 " << partial_read << " 字节:" << std::endl;
                                std::cout << small_buffer << std::endl;
                            }
                            
                            // 12. 尝试从偏移量开始读取
                            char offset_buffer[101] = {0};
                            // 在实际系统中，这里应该有seek操作，但我们的MiniFS可能没有实现
                            // 在这种情况下，可以多次read来跳过数据
                            if (partial_read > 0) {
                                std::cout << "\n步骤12: 尝试读取下一部分数据" << std::endl;
                                int next_read = fs.read(fd, offset_buffer, 100);
                                
                                if (next_read > 0) {
                                    offset_buffer[next_read] = '\0';
                                    std::cout << "成功从偏移位置读取 " << next_read << " 字节:" << std::endl;
                                    std::cout << offset_buffer << std::endl;
                                }
                            }
                            
                            fs.close(fd);
                        }
                    }
                }
            }
        } else {
            std::cout << "创建文件失败" << std::endl;
        }
        
        // 13. 列出目录内容
        std::cout << "\n步骤13: 列出测试目录中的文件" << std::endl;
        fs.listDir(test_dir_inum);
    } else {
        std::cout << "创建测试目录失败" << std::endl;
    }
    
    std::cout << "--- 文件操作测试结束 ---" << std::endl;
}

void test_user_operations(MiniFS& fs) {
    std::cout << "\n--- 开始用户管理功能测试 ---" << std::endl;

    // 1. 测试添加用户
    std::cout << "\n步骤1: 添加测试用户" << std::endl;
    fs.addUser("test_user1", "password1", 1001, 1001);
    fs.addUser("test_user2", "password2", 1002, 1002);

    // 2. 测试列出用户
    std::cout << "\n步骤2: 列出系统用户" << std::endl;
    fs.listUsers();

    // 3. 测试用户登录
    std::cout << "\n步骤3: 用户登录测试" << std::endl;
    bool login_result = fs.login("test_user1", "password1");
    std::cout << "用户登录" << (login_result ? "成功" : "失败") << std::endl;

    // 4. 测试获取当前用户
    std::cout << "\n步骤4: 获取当前用户信息" << std::endl;
    if (fs.isLoggedIn()) {
        const User& current_user = fs.getCurrentUser();
        std::cout << "当前用户: " << current_user.getUsername() 
                  << " (UID: " << current_user.getUid() 
                  << ", GID: " << current_user.getGid() << ")" << std::endl;
    } else {
        std::cout << "当前无用户登录" << std::endl;
    }

    // 5. 测试用户登出
    std::cout << "\n步骤5: 用户登出测试" << std::endl;
    bool logout_result = fs.logout();
    std::cout << "用户登出" << (logout_result ? "成功" : "失败") << std::endl;

    // 6. 测试错误情况
    std::cout << "\n步骤6: 测试错误情况" << std::endl;
    
    // 6.1. 使用错误密码登录
    std::cout << "尝试使用错误密码登录: ";
    login_result = fs.login("test_user1", "wrong_password");
    std::cout << (login_result ? "意外成功!" : "预期失败") << std::endl;
    
    // 6.2. 使用不存在的用户登录
    std::cout << "尝试使用不存在的用户登录: ";
    login_result = fs.login("non_existent_user", "password");
    std::cout << (login_result ? "意外成功!" : "预期失败") << std::endl;
    
    // 6.3. 尝试添加已存在的用户
    std::cout << "尝试添加已存在的用户: ";
    bool add_result = fs.addUser("test_user1", "new_password", 1003, 1003);
    std::cout << (add_result ? "意外成功!" : "预期失败") << std::endl;

    // 7. 清理测试状态
    std::cout << "\n步骤7: 确保登录状态一致" << std::endl;
    if (fs.isLoggedIn()) {
        fs.logout();
    }
    
    std::cout << "--- 用户管理功能测试结束 ---" << std::endl;
}


