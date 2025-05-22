#ifndef MINIFS_HPP
#define MINIFS_HPP

#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstring>

// 基本常量定义
//单个数据块的大小512字节-》 虚拟磁盘内数据块的数量 -》
constexpr int BLOCK_SIZE = 512; 
constexpr int BLOCK_COUNT = 1024;
constexpr int INODE_SIZE = 64;  //一个节点所占大小，可以知道：1块内有8(BLOCK_SIZE / INODE_SIZE)个i结点
constexpr int INODE_BLOCKS = 16;  //存放i节点的块数
constexpr int INODE_NUM = (INODE_BLOCKS * (BLOCK_SIZE / INODE_SIZE)); //i结点总数是128， 对应i结点位图需要128bit，即16字节
constexpr int INODE_START = 1; //i结点起始块号， 块0用于超级块等其他作用
constexpr int DATA_START = (INODE_START + INODE_BLOCKS); //数据区域的起始块号， 总块号-起始块号就是数据块数量：1024-17，即1007，对应1007bit，需要126字节
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
private:
    using Byte = uint8_t; //相当于typedef，给类型起了别名
    std::vector<Byte> disk;  // 虚拟磁盘，使用动态数组代替固定数组

public:
    // 文件系统状态枚举
    enum class FSStatus 
    { 
        OK = 0,  
        FAIL = -1, 
        CORRUPT = -2 
    };

    // 构造函数 - 初始化虚拟磁盘
    MiniFS();

    // 块读写方法
    void readBlock(int blockNum, void* buf); //读取一个数据块到内存缓冲区buf中
    void writeBlock(int blockNum, const void* buf);

    // 文件系统操作
    void format();                            // 格式化文件系统，最重要初始化函数，准备disk作为文件系统来使用
    FSStatus loadFS(const std::string& filename);  // 加载文件系统
    int saveFS(const std::string& filename);       // 保存文件系统

    // 目录操作
    void listRoot();                          // 列出根目录内容
    int checkFSConsistency();                 // 检查文件系统一致性
};

#endif // MINIFS_HPP
