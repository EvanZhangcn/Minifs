#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <iomanip> // 添加 iomanip 以支持 std::setw
#include <cstring>
#include <sstream>

// 前向声明 MiniFS 类，解决循环引用问题
class MiniFS;

// 定义用户类
class User {
private:
    std::string username;
    std::string password_hash; // 直接存储密码，没考虑用哈希值
    int uid;                   // 用户ID
    int gid;                   // 用户组ID

public:
    // 构造函数
    User(const std::string& uname, const std::string& pw, int uid, int gid)
        : username(uname), password_hash(pw), uid(uid), gid(gid) {}

    // 默认构造函数
    User() : username(""), password_hash(""), uid(-1), gid(-1) {}

    // 获取用户名
    const std::string& getUsername() const { return username; }

    // 获取用户ID
    int getUid() const { return uid; }

    // 获取组ID
    int getGid() const { return gid; }

    // 获取密码（直接访问，不做加密）
    const std::string& getPassword() const { return password_hash; }

    // 验证密码
    bool verifyPassword(const std::string& pw) const {
        return password_hash == pw;
    }

    // 检查用户是否有效
    bool isValid() const {
        return uid != -1;
    }
};

// 用户管理类
class UserManager {
private:
    std::unordered_map<std::string, User> users;  // 用户名到用户对象的映射
    std::unordered_map<int, User> user_ids;       // 用户ID到用户对象的映射
    User current_user;                            // 当前登录的用户
    bool someone_logged_in;                       // 是否有用户登录

public:
    // 构造函数
    UserManager() : someone_logged_in(false) {
        // 初始化默认管理员用户
        //用户名为root，密码为root， 设置uid为0，gid也为0
        addUser("root", "root", 0, 0);
    }

    // 复制构造函数
    UserManager(const UserManager& other)
        : users(other.users), user_ids(other.user_ids),
          current_user(other.current_user), someone_logged_in(other.someone_logged_in) {
    }

    // 添加用户
    bool addUser(const std::string& username, const std::string& password, int uid, int gid) {
        // 检查用户名是否已存在
        if (users.find(username) != users.end()) {
            std::cerr << "错误: 用户 '" << username << "' 已存在" << std::endl;
            return false;
        }

        // 检查UID是否已存在
        if (user_ids.find(uid) != user_ids.end()) {
            std::cerr << "错误: 用户ID " << uid << " 已被使用" << std::endl;
            return false;
        }

        // 创建并添加用户
        User new_user(username, password, uid, gid);
        users[username] = new_user;
        user_ids[uid] = new_user;

        std::cout << "成功添加用户: " << username << " (UID: " << uid << ", GID: " << gid << ")" << std::endl;
        return true;
    }

    // 用户登录
    bool login(const std::string& username, const std::string& password) {
        // 检查是否已有用户登录
        if (someone_logged_in) {
            std::cerr << "错误: 当前已有用户登录，请先登出" << std::endl;
            return false;
        }

        // 检查用户是否存在
        auto it = users.find(username);
        if (it == users.end()) {
            std::cerr << "错误: 用户 '" << username << "' 不存在" << std::endl;
            return false;
        }

        // 验证密码
        if (!it->second.verifyPassword(password)) {
            std::cerr << "错误: 密码不正确" << std::endl;
            return false;
        }

        // 设置当前用户
        current_user = it->second;
        someone_logged_in = true;

        std::cout << "用户 '" << username << "' 登录成功" << std::endl;
        return true;
    }

    // 用户登出
    bool logout() {
        if (!someone_logged_in) {
            std::cerr << "错误: 当前没有用户登录" << std::endl;
            return false;
        }

        std::cout << "用户 '" << current_user.getUsername() << "' 已登出" << std::endl;
        someone_logged_in = false;
        current_user = User(); // 重置为无效用户

        return true;
    }

    // 获取当前用户
    const User& getCurrentUser() const {
        return current_user;
    }

    // 检查是否有用户登录
    bool isLoggedIn() const {
        return someone_logged_in;
    }

    // 检查指定用户是否存在
    bool userExists(const std::string& username) const {
        return users.find(username) != users.end();
    }

    // 列出所有用户
    void listUsers() const {
        std::cout << "系统用户列表：" << std::endl;
        std::cout << std::left << std::setw(15) << "用户名" 
                  << std::setw(10) << "UID" 
                  << std::setw(10) << "GID" << std::endl;
        std::cout << std::string(35, '-') << std::endl;

        for (const auto& user_entry : users) {
            const User& user = user_entry.second;
            std::cout << std::left << std::setw(15) << user.getUsername()
                      << std::setw(10) << user.getUid()
                      << std::setw(10) << user.getGid() << std::endl;
        }
    }

    // 将用户数据保存到文件系统中
    bool saveUsersToFS(MiniFS* fs);

    // 从文件系统中加载用户数据
    bool loadUsersFromFS(MiniFS* fs);

    // 获取用户信息字符串（用于保存）
    std::string getUsersDataString() const;    // 从字符串解析用户信息（用于加载）
    bool parseUsersDataString(const std::string& data);

    // 清空所有用户数据（用于测试）
    void clearUsers();
};

#endif // USER_H
