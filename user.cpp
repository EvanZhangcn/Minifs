#include "user.hpp"
#include "minifs.hpp"
#include <sstream>

// 将用户数据保存到文件系统中
bool UserManager::saveUsersToFS(MiniFS* fs) {
    if (!fs) return false;

    // 首先检查/etc目录是否存在，不存在则创建
    int etc_dir_inum = fs->resolve_path_to_inum("/etc", MiniFS::ROOT_INUM_CONST);
    if (etc_dir_inum == MiniFS::INVALID_INUM_CONST) {
        // 创建/etc目录
        etc_dir_inum = fs->mkdir(MiniFS::ROOT_INUM_CONST, "etc");
        if (etc_dir_inum == MiniFS::INVALID_INUM_CONST) {
            std::cerr << "错误: 无法创建/etc目录" << std::endl;
            return false;
        }
    }

    // 获取用户数据字符串
    std::string user_data = getUsersDataString();

    // 创建/etc/passwd文件
    // 首先查看文件是否已存在，存在则删除
    int passwd_inum = fs->_lookup_in_directory(etc_dir_inum, "passwd");
    if (passwd_inum != MiniFS::INVALID_INUM_CONST) {
        fs->rm(etc_dir_inum, "passwd");
    }

    // 创建新文件
    passwd_inum = fs->create(etc_dir_inum, "passwd");
    if (passwd_inum == MiniFS::INVALID_INUM_CONST) {
        std::cerr << "错误: 无法创建/etc/passwd文件" << std::endl;
        return false;
    }

    // 打开文件
    int fd = fs->open(etc_dir_inum, "passwd", MiniFS::O_WRONLY);
    if (fd == -1) {
        std::cerr << "错误: 无法打开/etc/passwd文件写入" << std::endl;
        return false;
    }

    // 写入用户数据
    int bytes_written = fs->write(fd, user_data.c_str(), user_data.length());
    fs->close(fd);

    if (bytes_written != static_cast<int>(user_data.length())) {
        std::cerr << "错误: 写入/etc/passwd文件时发生错误" << std::endl;
        return false;
    }

    std::cout << "已成功保存 " << users.size() << " 个用户信息到文件系统" << std::endl;
    return true;
}

// 从文件系统中加载用户数据
bool UserManager::loadUsersFromFS(MiniFS* fs) {
    if (!fs) return false;

    // 查找/etc目录是否存在
    int etc_dir_inum = fs->resolve_path_to_inum("/etc", MiniFS::ROOT_INUM_CONST);
    if (etc_dir_inum == MiniFS::INVALID_INUM_CONST) {
        // 目录不存在，使用默认用户配置
        return false;
    }

    // 查找/etc/passwd文件是否存在
    int passwd_inum = fs->_lookup_in_directory(etc_dir_inum, "passwd");
    if (passwd_inum == MiniFS::INVALID_INUM_CONST) {
        // 文件不存在，使用默认用户配置
        return false;
    }

    // 打开文件
    int fd = fs->open(etc_dir_inum, "passwd", MiniFS::O_RDONLY);
    if (fd == -1) {
        return false;
    }

    // 读取文件内容
    std::string content;
    char buffer[1024];
    int bytes_read;
    int total_bytes_read = 0;
    const int max_file_size = 64 * 1024; // 限制最大文件大小为64KB
    
    while ((bytes_read = fs->read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        // 防止无限循环的安全检查
        total_bytes_read += bytes_read;
        if (total_bytes_read > max_file_size) {
            std::cerr << "错误: 文件过大，可能存在读取问题" << std::endl;
            fs->close(fd);
            return false;
        }
        
        buffer[bytes_read] = '\0';
        content += buffer;
        
        // 如果读取的字节数小于缓冲区大小，说明已到文件末尾
        if (bytes_read < static_cast<int>(sizeof(buffer) - 1)) {
            break;
        }
    }
    
    fs->close(fd);

    // 如果没有读取到任何内容，返回false
    if (content.empty()) {
        return false;
    }

    // 解析用户数据
    if (!parseUsersDataString(content)) {
        return false;
    }

    std::cout << "已成功从文件系统加载 " << users.size() << " 个用户信息" << std::endl;
    return true;
}

// 获取用户信息字符串（用于保存）
// 格式: username:password:uid:gid\n
std::string UserManager::getUsersDataString() const {
    std::stringstream ss;
    
    for (const auto& pair : users) {
        const User& user = pair.second;
        ss << user.getUsername() << ":"
           << user.getPassword() << ":" 
           << user.getUid() << ":" 
           << user.getGid() << "\n";
    }
    
    return ss.str();
}

// 从字符串解析用户信息（用于加载）
bool UserManager::parseUsersDataString(const std::string& data) {
    // 清空现有的用户数据
    users.clear();
    user_ids.clear();
    
    std::stringstream ss(data);
    std::string line;
    
    while (std::getline(ss, line)) {
        // 跳过空行
        if (line.empty()) continue;
        
        // 解析一行用户数据
        std::stringstream line_ss(line);
        std::string username, password, uid_str, gid_str;
        
        // 按照 : 分隔符读取字段
        if (!std::getline(line_ss, username, ':') ||
            !std::getline(line_ss, password, ':') ||
            !std::getline(line_ss, uid_str, ':') ||
            !std::getline(line_ss, gid_str)) {
            std::cerr << "错误: 解析用户数据行失败: " << line << std::endl;
            continue;
        }
        
        // 转换UID和GID为整数
        int uid, gid;
        try {
            uid = std::stoi(uid_str);
            gid = std::stoi(gid_str);
        } catch (const std::exception& e) {
            std::cerr << "错误: 用户ID转换失败: " << e.what() << std::endl;
            continue;
        }
        
        // 添加用户
        User new_user(username, password, uid, gid);
        users[username] = new_user;
        user_ids[uid] = new_user;
    }
    
    // 确保至少有root用户
    if (users.find("root") == users.end()) {
        addUser("root", "root", 0, 0);
    }
    
    return true;
}

// 清空所有用户数据（用于测试）
void UserManager::clearUsers() {
    users.clear();
    user_ids.clear();
    current_user = User(); // 重置为默认用户对象
    someone_logged_in = false;
    std::cout << "已清空所有用户数据" << std::endl;
}
