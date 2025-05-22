#include "minifs.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>

// 构造函数 - 初始化虚拟磁盘
MiniFS::MiniFS() {
    // 初始化虚拟磁盘大小并清零
    disk.resize(BLOCK_SIZE * BLOCK_COUNT, 0);
}

// 块读取方法
void MiniFS::readBlock(int blockNum, void* buf) 
{
    std::memcpy(buf, disk.data() + blockNum * BLOCK_SIZE, BLOCK_SIZE);
}

// 块写入方法
//将缓冲区的内容写入到指定块内(给定块号)
//eg： 超级块写入0号块
//
void MiniFS::writeBlock(int blockNum, const void* buf) 
{
    std::memcpy(disk.data() + blockNum * BLOCK_SIZE, buf, BLOCK_SIZE);
}

// 保存文件系统到本地文件
int MiniFS::saveFS(const std::string& filename) {
    try {
        // 创建备份文件名
        std::string backupFilename = filename + ".bak";
        
        // 如果原文件存在，先创建备份
        std::ifstream existingFile(filename, std::ios::binary);
        if (existingFile) {
            existingFile.close(); // 关闭检查用的流
            
            // 复制原文件到备份文件
            std::ifstream src(filename, std::ios::binary);
            std::ofstream dst(backupFilename, std::ios::binary | std::ios::trunc);
            
            if (src && dst) {
                dst << src.rdbuf(); // 使用rdbuf()进行高效、健壮的复制
                if (src.eof() && !dst.fail()) { // 检查复制是否成功完成
                    std::cout << "已创建备份: " << backupFilename << std::endl;
                } else {
                    std::cerr << "创建备份时发生错误: " << backupFilename << std::endl;
                    // 可以选择是否在这里返回错误，或者继续尝试保存主文件
                }
            } else {
                std::cerr << "无法打开用于备份的文件流: src=" << src.good() << ", dst=" << dst.good() << std::endl;
            }
        }

        // 打开文件进行写入
        std::ofstream outFile(filename, std::ios::binary | std::ios::trunc);
        if (!outFile) {
            std::cerr << "无法打开文件进行保存: " << filename << std::endl;
            return -1;
        }
        
        // 写入文件系统数据
        outFile.write(reinterpret_cast<char*>(disk.data()), disk.size());
        
        // 确保数据写入到磁盘
        outFile.flush();
        
        if (!outFile) {
            std::cerr << "写入磁盘镜像时出错: 预期写入 " << disk.size() << " 字节" << std::endl;
            return -1;
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "保存文件系统时发生异常: " << e.what() << std::endl;
        return -1;
    }
}

// 加载文件系统到内存
MiniFS::FSStatus MiniFS::loadFS(const std::string& filename) {
    try {
        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile) {
            std::cerr << "无法打开文件 " << filename << std::endl;
            return FSStatus::FAIL;
        }

        // 检查文件大小
        inFile.seekg(0, std::ios::end);
        std::streampos fileSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);
        
        if (fileSize != BLOCK_SIZE * BLOCK_COUNT) {
            std::cerr << "文件大小不匹配: 预期 " << (BLOCK_SIZE * BLOCK_COUNT) 
                      << " 字节, 实际 " << fileSize << " 字节" << std::endl;
            return FSStatus::CORRUPT;
        }
        
        // 读取文件内容到虚拟磁盘
        inFile.read(reinterpret_cast<char*>(disk.data()), disk.size());
        if (!inFile || inFile.gcount() != disk.size()) {
            std::cerr << "读取磁盘镜像时出错: 预期读取 " << disk.size() 
                      << " 字节, 实际读取 " << inFile.gcount() << " 字节" << std::endl;
            return FSStatus::CORRUPT;
        }
        
        // 验证超级块基本信息
        superblock sb;
        std::memcpy(&sb, disk.data(), sizeof(superblock));
        if (sb.size != BLOCK_COUNT || sb.inode_start != INODE_START || sb.data_start != DATA_START) {
            std::cerr << "文件系统超级块信息不一致，可能已损坏" << std::endl;
            return FSStatus::CORRUPT;
        }
        
        return FSStatus::OK;
    }
    catch (const std::exception& e) {
        std::cerr << "加载文件系统时发生异常: " << e.what() << std::endl;
        return FSStatus::FAIL;
    }
}

// 格式化文件系统
void MiniFS::format() 
{
    // 1. 初始化超级块
    superblock sb;
    sb.size = BLOCK_COUNT;
    sb.ninodes = INODE_NUM;
    sb.nblocks = BLOCK_COUNT - DATA_START;
    sb.inode_start = INODE_START;
    sb.data_start = DATA_START;
    writeBlock(0, &sb); // 0号块写超级块

    // 2. 初始化i-节点区
    //初始化inode table
    //将一个i节点初始化，以此为副本，用for循环对后续所有i结点进行同样操作
    dinode inode;
    std::memset(&inode, 0, sizeof(dinode));
    inode.type = T_FREE;
    
    for (int i = 0; i < INODE_NUM; ++i) {
        int block = INODE_START + (i * INODE_SIZE) / BLOCK_SIZE;
        int offset = (i * INODE_SIZE) % BLOCK_SIZE;
        //缓冲区为一块的大小
        Byte buf[BLOCK_SIZE];
        readBlock(block, buf);
        std::memcpy(buf + offset, &inode, sizeof(dinode));
        writeBlock(block, buf);
    }

    // 3. 创建根目录i-节点（1号i-节点）,并分配指向数据块起始
    dinode rootInode;
    std::memset(&rootInode, 0, sizeof(dinode));
    int rootInum = 1; //根目录的i-节点号通常约定为1
    int rootDataBlock = DATA_START;
    rootInode.type = T_DIR;
    rootInode.size = 2 * sizeof(dirent); //先分配两个文件项dirent: 一个给.另一个给..
    rootInode.nlink = 2;
    rootInode.addrs[0] = rootDataBlock;
    //更新Inodes
    // 写回根目录i-节点，1号结点对应第一块数据块
    int block = INODE_START + (rootInum * INODE_SIZE) / BLOCK_SIZE;
    int offset = (rootInum * INODE_SIZE) % BLOCK_SIZE;
    Byte buf[BLOCK_SIZE];
    readBlock(block, buf);
    std::memcpy(buf + offset, &rootInode, sizeof(dinode));
    writeBlock(block, buf);
    //更新对应的数据块
    // 4. 初始化根目录数据块
    dirent entries[2];
    entries[0].inum = rootInum;//当前目录inum是1
    std::strcpy(entries[0].name, ".");
    entries[1].inum = rootInum;//根目录下，父目录指向本身，所以inum也是1
    std::strcpy(entries[1].name, "..");
    writeBlock(rootDataBlock, entries);



}

// 列出根目录内容
void MiniFS::listRoot() 
{
    std::cout << "正在列出根目录内容..." << std::endl;
    
    try {
        // 1. 读取根目录i-节点 (XV6中根目录通常是inode 1)
        int rootInum = 1; 
        int block_for_root_inode = INODE_START + (rootInum * INODE_SIZE) / BLOCK_SIZE;
        int offset_in_block = (rootInum * INODE_SIZE) % BLOCK_SIZE;
        
        std::cout << "读取根i-节点 (" << rootInum << ") 位置: block=" << block_for_root_inode << ", offset=" << offset_in_block << std::endl;
        
        Byte buf[BLOCK_SIZE];
        std::memset(buf, 0, sizeof(buf)); // 清零缓冲区
        readBlock(block_for_root_inode, buf);
        
        dinode rootInode;
        std::memset(&rootInode, 0, sizeof(dinode)); // 清零防止脏数据
        std::memcpy(&rootInode, buf + offset_in_block, sizeof(dinode));
        
        std::cout << "根i-节点信息: type=" << rootInode.type 
                << ", size=" << rootInode.size 
                << ", first_block=" << rootInode.addrs[0] << std::endl;
        
        if (rootInode.type != T_DIR) {
            std::cerr << "错误：根i-节点不是目录类型！ (类型为: " << rootInode.type << ")" << std::endl;
            return;
        }
        
        // 2. 读取根目录数据块
        dirent entries[BLOCK_SIZE / sizeof(dirent)];
        std::memset(entries, 0, sizeof(entries)); // 清零缓冲区
        
        std::cout << "读取根目录数据块: " << rootInode.addrs[0] << std::endl;
        readBlock(rootInode.addrs[0], entries);
        
        // 安全检查目录项大小
        int entryCount = rootInode.size / sizeof(dirent);
        if (entryCount <= 0 || entryCount > BLOCK_SIZE / sizeof(dirent)) {
            std::cerr << "错误：根目录项数量异常: " << entryCount << std::endl;
            return;
        }
        
        // 3. 打印目录项
        std::cout << "根目录内容：" << std::endl;
        for (int i = 0; i < entryCount; ++i) {
            std::cout << std::left << std::setw(15) << entries[i].name 
                    << "(i-节点号: " << entries[i].inum << ")" << std::endl;
        }
        
        std::cout << "根目录列表完成" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "列出根目录时发生异常: " << e.what() << std::endl;
    }
}

// 检查文件系统一致性
int MiniFS::checkFSConsistency() {
    std::cout << "开始检查文件系统一致性..." << std::endl;
    
    try {
        // 读取超级块
        superblock sb;
        std::memset(&sb, 0, sizeof(superblock)); // 清零防止脏数据
        readBlock(0, &sb);
        
        // 打印超级块信息进行调试
        std::cout << "超级块信息: size=" << sb.size 
                << ", ninodes=" << sb.ninodes 
                << ", nblocks=" << sb.nblocks 
                << ", inode_start=" << sb.inode_start 
                << ", data_start=" << sb.data_start << std::endl;
        
        // 基本检查
        if (sb.size != BLOCK_COUNT || sb.inode_start != INODE_START || sb.data_start != DATA_START) {
            std::cout << "超级块信息不匹配: 预期size=" << BLOCK_COUNT 
                    << ", inode_start=" << INODE_START 
                    << ", data_start=" << DATA_START << std::endl;
            return -1;
        }
        
        // 检查根目录是否存在 (XV6中根目录通常是inode 1)
        int rootInum = 1;
        // 计算根i-节点所在的块和块内偏移
        int block_for_root_inode = INODE_START + (rootInum * INODE_SIZE) / BLOCK_SIZE;
        int offset_in_block = (rootInum * INODE_SIZE) % BLOCK_SIZE;
        
        std::cout << "读取根i-节点 (" << rootInum << ") : block=" << block_for_root_inode << ", offset=" << offset_in_block << std::endl;
        
        Byte buf[BLOCK_SIZE];
        std::memset(buf, 0, sizeof(buf)); // 清零缓冲区
        readBlock(block_for_root_inode, buf);
        
        dinode rootInode;
        std::memset(&rootInode, 0, sizeof(dinode)); // 清零防止脏数据
        std::memcpy(&rootInode, buf + offset_in_block, sizeof(dinode));
        
        // 打印根i-节点信息进行调试
        std::cout << "根i-节点信息: type=" << rootInode.type 
                << ", nlink=" << rootInode.nlink 
                << ", size=" << rootInode.size 
                << ", addrs[0]=" << rootInode.addrs[0] << std::endl;
        
        if (rootInode.type != T_DIR) {
            std::cout << "根i-节点类型错误: 应为目录(" << T_DIR 
                     << "), 实际为" << rootInode.type << std::endl;
            return -1;
        }
        
        std::cout << "文件系统一致性检查通过!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "检查文件系统一致性时发生异常: " << e.what() << std::endl;
        return -1;
    }
}
