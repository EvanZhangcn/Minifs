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
    enum class FSStatus { OK, FAIL, CORRUPT, NOT_FOUND };

    static const int ROOT_INUM_CONST = 1; // 根目录的i-节点号通常约定为1
    static const int INVALID_INUM_CONST = -1; // 表示无效或未找到的i-节点号

    MiniFS();
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


private:
    // 新增的私有方法
    bool _get_inode(int inum, dinode& node_out);
    int _lookup_in_directory(int dir_inum, const std::string& name);
    std::vector<Byte> disk;
};

#endif // MiniFS_HPP
