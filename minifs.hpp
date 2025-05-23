#ifndef MINIFS_HPP
#define MINIFS_HPP
#include <iomanip> // std::setw 要用这个头文件
#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstring>
#include <sstream>
#include "user.hpp" // 包含完整的 user.hpp

typedef unsigned char Byte;
//位图块定义
constexpr int SUPERBLOCK_START = 0;
constexpr int INODE_BITMAP_BLOCK_START = SUPERBLOCK_START + 1;
constexpr int INODE_BITMAP_BLOCK_COUNT = 1;
constexpr int DATA_BITMAP_BLOCK_START = INODE_BITMAP_BLOCK_START + INODE_BITMAP_BLOCK_COUNT;
constexpr int DATA_BITMAP_BLOCK_COUNT = 1;


// 基本常量定义
//单个数据块的大小512字节-》 虚拟磁盘内数据块的数量 -》
constexpr int BLOCK_SIZE = 512; 
constexpr int BLOCK_COUNT = 1024;
constexpr int INODE_SIZE = 64;  //一个节点所占大小，可以知道：1块内有8(BLOCK_SIZE / INODE_SIZE)个i结点
constexpr int INODE_BLOCKS = 16;  //存放i节点的块数
constexpr int INODE_NUM = (INODE_BLOCKS * (BLOCK_SIZE / INODE_SIZE)); //i结点总数是128， 对应i结点位图需要128bit，即16字节
constexpr int INODE_START = DATA_BITMAP_BLOCK_START + DATA_BITMAP_BLOCK_COUNT; //i结点起始块号，对应块3， 块0用于超级块
constexpr int DATA_START = (INODE_START + INODE_BLOCKS); //数据区域的起始块号， 总块号-起始块号就是数据块数量：1024-17，即1007，对应1007bit，需要126字节
constexpr int DATA_BLOCKS_NUM = BLOCK_COUNT - DATA_START;
constexpr int DIRSIZ = 28; //目录项中文件名的最大长度



// 文件类型常量
constexpr int T_FREE = 0;
constexpr int T_FILE = 1; //该i节点表示文件
constexpr int T_DIR  = 2; //该i节点表示目录

// 超级块结构体
//初始化用上面预先设定好的常量constexpr
struct superblock {
    int size;           // 块总数
    int ninodes;        // i-节点数
    int nblocks;        // 数据块数
    int inode_start;    // i-节点区起始块
    int data_start;     // 数据区起始块
    //记录新增的位图的信息
    int inode_bitmap_start_block;
    int data_bitmap_start_block;
};

// 磁盘i-节点结构体
struct dinode {
    int16_t type;       // 文件类型
    int16_t nlink;      // 链接数
    int size;           // 文件大小
    int addrs[8];       // 数据块指针
};

// 目录项结构体,目录就是一堆dirent结构体，组成的链表
struct dirent {
    //形成(inum, name)对
    int inum;           // i-节点号
    char name[DIRSIZ];  // 文件名
};

// 文件系统类
class MiniFS {
public:
    static const int O_RDONLY = 0x0001; // 只读  对应01
    static const int O_WRONLY = 0x0002; // 只写  对应10
    static const int O_RDWR   = 0x0003; // 读写 (O_RDONLY | O_WRONLY)  对应00
    static const int O_CREATE = 0x0100; // 如果不存在则创建


    enum class FSStatus { OK, FAIL, CORRUPT, NOT_FOUND };
    static const int ROOT_INUM_CONST = 1; // 根目录的i-节点号通常约定为1
    static const int INVALID_INUM_CONST = -1; // 表示无效或未找到的i-节点号

    // 用户管理相关方法
    UserManager userManager;  // 用户管理器，直接作为成员对象

    // 构造函数
    MiniFS();
    ~MiniFS(); // 添加析构函数处理资源
    
    void readBlock(int blockNum, void* buf);
    void writeBlock(int blockNum, const void* buf);
    int saveFS(const std::string& filename);
    FSStatus loadFS(const std::string& filename);
    void format();
    int mkdir(int parent_dir_inum, const char* name); // 创建目录的核心实现
    void listDir(int dir_inum);                       // 列出指定inum目录的内容
    void listRoot();                                  // 列出根目录内容
    int checkFSConsistency();


    // 位图操作 (保持现有)
    void set_bit(int bitmap_block_start, int index);
    void clear_bit(int bitmap_block_start, int index);
    bool test_bit(int bitmap_block_start, int index);
    int find_free_bit(int bitmap_block_start, int total_bits, int min_allowed_index = 0);
    
    int balloc();
    void bfree(int absolute_block_num);
    int ialloc(int16_t type);
    void ifree(int inum);

    // 路径解析功能
    int resolve_path_to_inum(const std::string& path, int base_inum = ROOT_INUM_CONST);
    bool _get_inode(int inum, dinode& node_out);
    int _lookup_in_directory(int dir_inum, const std::string& name);    //create
    
    
    // 文件描述符结构体
    struct file_descriptor {
        int inum;          // 关联的i-节点号
        int mode;          // 打开模式(O_RDONLY, O_WRONLY, O_RDWR)
        int position;      // 当前文件读写位置
        bool is_used;      // 文件描述符是否在使用中
    };
    
    // 文件操作函数
    int create(int parent_dir_inum, const char* name);
    int open(int parent_dir_inum, const char* name, int flags);
    int close(int fd);
    int read(int fd, void* buf, int count);
    int write(int fd, const void* buf, int count);

    // 删除目录
    int rmdir(int parent_dir_inum, const char* name);
    
    // 删除文件
    int rm(int parent_dir_inum, const char* name);

    // 用户登录
    bool login(const std::string& username, const std::string& password);

    // 用户登出
    bool logout();

    // 添加用户
    bool addUser(const std::string& username, const std::string& password, int uid, int gid);

    // 列出所有用户
    void listUsers();

    // 检查当前是否有用户登录
    bool isLoggedIn() const;

    // 获取当前用户
    const User& getCurrentUser() const;

    // 保存用户数据到文件系统
    bool saveUserData();
    
    // 从文件系统加载用户数据
    bool loadUserData();
    
    // 在格式化时保存用户数据
    void formatWithUserPreservation();

    // 清除用户数据
    void clearUserData(); // 添加这个方法声明

private:
    std::vector<Byte> disk;
    // 文件描述符表
    static const int MAX_OPEN_FILES = 16;
    file_descriptor fd_table[MAX_OPEN_FILES];
};

#endif // MiniFS_HPP
