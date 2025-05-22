#include "minifs.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>

// 构造函数 - 初始化虚拟磁盘
MiniFS::MiniFS() {
    try {
        // 变量，用于确保错误消息清晰可见
        size_t expected_size = static_cast<size_t>(BLOCK_SIZE) * BLOCK_COUNT;
        
        // 输出配置信息
        std::cout << "文件系统配置: BLOCK_SIZE=" << BLOCK_SIZE 
                  << ", BLOCK_COUNT=" << BLOCK_COUNT 
                  << ", 总大小=" << expected_size << " 字节" << std::endl;

        // 初始化虚拟磁盘大小并清零
        std::cout << "正在初始化虚拟磁盘..." << std::endl;
        disk.clear(); // 确保vector为空
        disk.reserve(expected_size); // 预分配空间
        disk.resize(expected_size, 0); // 调整大小并初始化为0
        
        // 验证内存分配
        if (disk.size() != expected_size) {
            std::cerr << "错误：虚拟磁盘初始化失败，实际大小: " << disk.size() 
                      << "，预期大小: " << expected_size << " 字节" << std::endl;
        } else {
            std::cout << "虚拟磁盘初始化成功，大小: " << disk.size() << " 字节" 
                      << "，数据指针: " << (void*)disk.data() << std::endl;
            
            // 额外验证
            if (disk.data() == nullptr) {
                std::cerr << "警告：disk.data() 返回空指针，这可能导致后续操作失败" << std::endl;
                disk.resize(0); // 清空
                disk.resize(expected_size, 0); // 重新尝试分配
                std::cout << "重新分配后指针: " << (void*)disk.data() << std::endl;
            }
        }
    } catch (const std::bad_alloc& e) {
        std::cerr << "内存分配失败: " << e.what() << std::endl;
        // 尝试分配较小的空间
        try {
            disk.resize(BLOCK_SIZE * 10, 0); // 只分配10个块
            std::cerr << "分配了较小的临时空间: " << disk.size() 
                     << " 字节，可能导致功能受限" << std::endl;
        } catch (...) {
            std::cerr << "无法分配任何磁盘空间，程序可能无法正常工作" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "虚拟磁盘初始化时发生异常: " << e.what() << std::endl;
    }
}

// 块读取方法
void MiniFS::readBlock(int blockNum, void* buf) 
{
    std::cout << "【DEBUG】开始读取块 " << blockNum << std::endl;
    std::cout.flush(); // 确保输出被刷新
    
    try {
        // 增加安全检查，确保不会越界访问
        if (blockNum < 0 || blockNum >= BLOCK_COUNT) {
            std::cerr << "错误: readBlock 尝试读取无效块号: " << blockNum 
                    << " (有效范围: 0-" << (BLOCK_COUNT-1) << ")" << std::endl;
            // 清零缓冲区而不是尝试读取无效内存
            std::memset(buf, 0, BLOCK_SIZE);
            return;
        }
        
        // 检查 disk 向量
        std::cout << "【DEBUG】检查 disk 向量: size=" << disk.size() 
                << ", capacity=" << disk.capacity() 
                << ", empty=" << (disk.empty() ? "true" : "false") << std::endl;
        std::cout.flush();
        
        if (disk.empty()) {
            std::cerr << "错误: disk 向量为空，无法读取任何数据" << std::endl;
            std::memset(buf, 0, BLOCK_SIZE);
            return;
        }
        
        // 确保 disk 已经正确初始化并且有足够的大小
        if (disk.size() < (blockNum + 1) * BLOCK_SIZE) {
            std::cerr << "错误: readBlock 尝试读取超出disk范围的数据! blockNum=" << blockNum 
                    << ", 需要访问位置=" << (blockNum + 1) * BLOCK_SIZE 
                    << ", disk.size()=" << disk.size() << std::endl;
            // 清零缓冲区而不是尝试读取无效内存
            std::memset(buf, 0, BLOCK_SIZE);
            return;
        }
        
        // 检查 data 指针有效性
        if (disk.data() == nullptr) {
            std::cerr << "错误: disk.data() 返回空指针" << std::endl;
            std::memset(buf, 0, BLOCK_SIZE);
            return;
        }
        
        // 计算源地址
        const void* src_addr = disk.data() + blockNum * BLOCK_SIZE;
        std::cout << "【DEBUG】源地址: " << src_addr << std::endl;
        std::cout.flush();
        
        // 执行复制
        std::cout << "【DEBUG】执行复制: 从 " << src_addr << " 到 " << buf << ", 大小=" << BLOCK_SIZE << " 字节" << std::endl;
        std::cout.flush();
        std::memcpy(buf, src_addr, BLOCK_SIZE);
        std::cout << "【DEBUG】块 " << blockNum << " 读取成功" << std::endl;
        std::cout.flush();
    }
    catch (const std::exception& e) {
        std::cerr << "readBlock 发生异常: " << e.what() << std::endl;
        std::memset(buf, 0, BLOCK_SIZE);
    }
    catch (...) {
        std::cerr << "readBlock 发生未知异常" << std::endl;
        std::memset(buf, 0, BLOCK_SIZE);
    }
}

// 块写入方法
//将缓冲区的内容写入到指定块内(给定块号)
//eg： 超级块写入0号块
//
void MiniFS::writeBlock(int blockNum, const void* buf) 
{
    // 增加安全检查，确保不会越界访问
    if (blockNum < 0 || blockNum >= BLOCK_COUNT) {
        std::cerr << "错误: writeBlock 尝试写入无效块号: " << blockNum 
                  << " (有效范围: 0-" << (BLOCK_COUNT-1) << ")" << std::endl;
        return;
    }
    
    // 确保 disk 已经正确初始化并且有足够的大小
    if (disk.size() < (blockNum + 1) * BLOCK_SIZE) {
        std::cerr << "错误: writeBlock 尝试写入超出disk范围的数据! blockNum=" << blockNum 
                  << ", 需要访问位置=" << (blockNum + 1) * BLOCK_SIZE 
                  << ", disk.size()=" << disk.size() << std::endl;
        
        // 如果磁盘大小不足，尝试调整大小
        try {
            disk.resize((blockNum + 1) * BLOCK_SIZE, 0);
            std::cout << "已自动扩展磁盘大小至: " << disk.size() << " 字节" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "扩展磁盘大小失败: " << e.what() << std::endl;
            return;
        }
    }
    
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
    Byte buf[BLOCK_SIZE];//临时变量，用作初始化
    // 1. 初始化超级块
    superblock sb;
    sb.size = BLOCK_COUNT;
    sb.ninodes = INODE_NUM;
    sb.nblocks = DATA_BLOCKS_NUM;
    sb.inode_start = INODE_START;
    sb.data_start = DATA_START;
    sb.inode_bitmap_start_block = INODE_BITMAP_BLOCK_START;
    sb.data_bitmap_start_block = DATA_BITMAP_BLOCK_START;
    writeBlock(0, &sb); // 0号块写超级块


    //5.  初始化i节点位图块和数据块位图块，对一块全部置0
    //必须放在前面，因为后面设置根节点，对应位图位置也要置1了
    //Byte buf[BLOCK_SIZE];//临时变量，用作初始化
    std::memset(buf, 0, BLOCK_SIZE);
    for (int i = 0; i < INODE_BITMAP_BLOCK_COUNT; i++)
    {
        writeBlock(INODE_BITMAP_BLOCK_START + i, buf);
    }
    for (int i = 0; i < DATA_BITMAP_BLOCK_COUNT; i++)
    {
        writeBlock(DATA_BITMAP_BLOCK_START + i, buf);
    }


    //2. 初始化i-节点区
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
    set_bit(INODE_BITMAP_BLOCK_START, rootInum);

    //更新Inodes
    // 写回根目录i-节点，1号结点对应第一块数据块
    int block = INODE_START + (rootInum * INODE_SIZE) / BLOCK_SIZE;
    int offset = (rootInum * INODE_SIZE) % BLOCK_SIZE;
    readBlock(block, buf);
    std::memcpy(buf + offset, &rootInode, sizeof(dinode));
    writeBlock(block, buf);
    set_bit(DATA_BITMAP_BLOCK_START, 0);// 标记数据区的第0个块已使用


    //更新对应的数据块
    // 4. 初始化根目录数据块
    dirent entries[2];
    entries[0].inum = rootInum;//当前目录inum是1
    std::strcpy(entries[0].name, ".");
    entries[1].inum = rootInum;//根目录下，父目录指向本身，所以inum也是1
    std::strcpy(entries[1].name, "..");
    writeBlock(rootDataBlock, entries);


}




/**
 * @param parent_dir_inum 父目录的i-节点号
 * @param name 要创建的新目录名称
 * @return int 成功时返回新目录的i-节点号，
 *             失败时返回-1（如名称过长或父目录不存在）
 * @note 函数执行流程：
 * 1. 检查目录名是否合法（长度不超过DIRSIZ限制）
 * 2. 检查父目录是否存在
 * 3. 分配新的i-节点（使用ialloc(T_DIR), balloc把数据区也分配了
 * 4. 更新父目录的目录项：
 *    - 根据parent_dir_inum计算在磁盘i-节点区的位置
 *    - 获取父目录i-节点指针
 *    - 通过i-节点中的address访问数据块
 * 5. 初始化新目录内容：
 *    - 添加"."和".."目录项
 * 6. 更新父目录的i-节点信息（如链接数等）
 * 7. 返回新创建目录的i-节点号
 */
int MiniFS::mkdir(int parent_dir_inum, const char* name)
{
    std::cout << "开始创建目录: " << name << " (parent inum: " << parent_dir_inum << ")" << std::endl;
    
    if (strlen(name) >= DIRSIZ) {
        std::cerr << "错误: 目录名称过长 (最大长度: " << DIRSIZ-1 << " 字符)" << std::endl;
        return -1;
    }

    // 1. 读取父目录i-节点
    int parent_block = INODE_START + (parent_dir_inum * INODE_SIZE) / BLOCK_SIZE;
    int parent_offset = (parent_dir_inum * INODE_SIZE) % BLOCK_SIZE;
    
    //从存放i节点的块中拿出数据，存到parent_inode中
    Byte buf[BLOCK_SIZE];
    readBlock(parent_block, buf);
    dinode parent_inode;
    std::memcpy(&parent_inode, buf + parent_offset, sizeof(dinode));
    
    // 确认父节点是一个目录
    if (parent_inode.type != T_DIR) {
        std::cerr << "错误: 父i-节点不是目录类型 (type = " << parent_inode.type << ")" << std::endl;
        return -1;
    }
    
    // 2. 检查同名冲突,目录项就是个大数组，遍历每一个目录项的名字，查看是否有重名
    int entries_count = parent_inode.size / sizeof(dirent);
    dirent entries[BLOCK_SIZE / sizeof(dirent)];
    readBlock(parent_inode.addrs[0], entries);  // 简化: 假设目录项都在第一个数据块
    
    for (int i = 0; i < entries_count; i++) {
        if (std::strcmp(entries[i].name, name) == 0) {
            std::cerr << "错误: 父目录中已存在同名项 '" << name << "'" << std::endl;
            return -1;
        }
    }
    
    // 3. 分配新目录的i-节点
    int child_dir_inum = ialloc(T_DIR);
    if (child_dir_inum == -1) {
        std::cerr << "错误: 无法分配i-节点" << std::endl;
        return -1;
    }
    
    // 4. 分配新目录的数据块
    int child_dir_data_block = balloc();
    if (child_dir_data_block == -1) {
        std::cerr << "错误: 无法分配数据块" << std::endl;
        ifree(child_dir_inum); // 释放之前分配的i-节点
        return -1;
    }
    
    // 5. 初始化新目录的i-节点
    int child_dir_block = INODE_START + (child_dir_inum * INODE_SIZE) / BLOCK_SIZE;
    int child_dir_offset = (child_dir_inum * INODE_SIZE) % BLOCK_SIZE;
    
    readBlock(child_dir_block, buf);
    dinode* child_dir_inode = reinterpret_cast<dinode*>(buf + child_dir_offset);
    
    child_dir_inode->type = T_DIR;
    child_dir_inode->size = 2 * sizeof(dirent);  // 包含 . 和 .. 两个条目
    child_dir_inode->nlink = 2;  // 自身的 . 和来自父目录的链接
    child_dir_inode->addrs[0] = child_dir_data_block;
    writeBlock(child_dir_block, buf);
    
    // 6. 初始化新目录的数据块 (创建 . 和 .. 条目)
    dirent child_dir_entries[2];
    // "." 条目，指向自身
    child_dir_entries[0].inum = child_dir_inum;
    std::strcpy(child_dir_entries[0].name, ".");
    // ".." 条目，指向父目录
    child_dir_entries[1].inum = parent_dir_inum;
    std::strcpy(child_dir_entries[1].name, "..");
    writeBlock(child_dir_data_block, child_dir_entries);
    
    // 7. 在父目录中添加新目录条目
    // 简化：假设父目录数据块有足够空间
    dirent new_entry;
    new_entry.inum = child_dir_inum;
    std::strcpy(new_entry.name, name);
    entries[entries_count] = new_entry; //增加这个entry

    
    // 8. 写回更新后的父目录数据
    writeBlock(parent_inode.addrs[0], entries);
    
    // 9. 写回更新后的父目录i-节点
    parent_inode.size += sizeof(dirent);  // 增加父目录大小
    parent_inode.nlink++;  // 增加父目录链接数 (新目录的 .. 链接到父目录)
    std::memcpy(buf + parent_offset, &parent_inode, sizeof(dinode));
    writeBlock(parent_block, buf);
    
    std::cout << "成功创建目录: " << name << " (inum: " << child_dir_inum << ")" << std::endl;
    
    return child_dir_inum;
}

// 列出指定目录的内容
void MiniFS::listDir(int dir_inum) 
{
    std::cout << "正在列出目录内容 (inum: " << dir_inum << ")..." << std::endl;
    
    try {
        // 1. 读取目录的i-节点
        int block = INODE_START + (dir_inum * INODE_SIZE) / BLOCK_SIZE;
        int offset = (dir_inum * INODE_SIZE) % BLOCK_SIZE;
        
        Byte buf[BLOCK_SIZE];
        std::memset(buf, 0, sizeof(buf));
        readBlock(block, buf);
        
        dinode dir_inode;
        std::memcpy(&dir_inode, buf + offset, sizeof(dinode));
        
        // 确保是目录类型
        if (dir_inode.type != T_DIR) {
            std::cerr << "错误：i-节点 " << dir_inum << " 不是目录类型 (type: " << dir_inode.type << ")" << std::endl;
            return;
        }
        
        // 2. 读取目录数据块
        int entries_count = dir_inode.size / sizeof(dirent);
        dirent entries[BLOCK_SIZE / sizeof(dirent)];
        std::memset(entries, 0, sizeof(entries));
        readBlock(dir_inode.addrs[0], entries);  //假设目录项都在第一个数据块
        
        // 3. 打印目录项
        std::cout << "目录内容 (共 " << entries_count << " 项)：" << std::endl;
        std::cout << std::left << std::setw(30) << "名称" << "i-节点号" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        //遍历目录项
        for (int i = 0; i < entries_count; ++i) 
        {
            std::cout << std::left << std::setw(30) << entries[i].name 
                      << entries[i].inum << std::endl;
        }
        
        std::cout << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "列出目录时发生异常: " << e.what() << std::endl;
    }
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
        
        // 增加安全检查，确保块号有效
        if (block_for_root_inode < 0 || block_for_root_inode >= BLOCK_COUNT) {
            std::cerr << "错误：无效的块号: " << block_for_root_inode << std::endl;
            return -1;
        }
        
        Byte buf[BLOCK_SIZE];
        std::memset(buf, 0, sizeof(buf)); // 清零缓冲区
          // 输出一些调试信息
        std::cout << "即将读取block " << block_for_root_inode << "，disk.size() = ";
        std::cout.flush(); // 确保输出被刷新
        std::cout << disk.size() << std::endl;
        
        // 检查disk向量是否为空
        if (disk.empty()) {
            std::cerr << "错误: disk向量为空!" << std::endl;
            return -1;
        }
        
        // 重新计算安全的块大小，避免可能的访问越界
        size_t safe_block_size = std::min(static_cast<size_t>(BLOCK_SIZE), 
                                         disk.size() - block_for_root_inode * BLOCK_SIZE);
        
        // 使用安全的内存访问方式
        if (block_for_root_inode * BLOCK_SIZE < disk.size()) {
            std::memcpy(buf, disk.data() + block_for_root_inode * BLOCK_SIZE, safe_block_size);
            if (safe_block_size < BLOCK_SIZE) {
                // 如果读取的块大小小于完整块大小，将剩余部分填充为0
                std::memset(buf + safe_block_size, 0, BLOCK_SIZE - safe_block_size);
                std::cerr << "警告: 块 " << block_for_root_inode << " 只能读取 " 
                          << safe_block_size << " 字节 (不是完整的块)" << std::endl;
            }
        } else {
            std::cerr << "错误：尝试读取超出disk范围的数据! block=" << block_for_root_inode 
                      << ", offset=" << block_for_root_inode * BLOCK_SIZE 
                      << ", disk.size()=" << disk.size() << std::endl;
            std::memset(buf, 0, BLOCK_SIZE); // 清零缓冲区
            return -1;
        }
        
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

//位图操作函数
void MiniFS::set_bit(int bitmap_block_start, int index)
{
    int byte_index = index / 8;
    int bit_offset = index % 8;
    int block_offset = byte_index / BLOCK_SIZE;
    int byte_in_block = byte_index % BLOCK_SIZE; 

    Byte buf[BLOCK_SIZE];
    readBlock(bitmap_block_start + block_offset, buf);
    buf[byte_in_block] |= (1 << bit_offset);
    writeBlock(bitmap_block_start + block_offset, buf);
}

void MiniFS::clear_bit(int bitmap_block_start, int index)
{
    int byte_index = index / 8;
    int bit_offset = index % 8;
    int block_offset = byte_index / BLOCK_SIZE;
    int byte_in_block = byte_index % BLOCK_SIZE; 

    Byte buf[BLOCK_SIZE];
    readBlock(bitmap_block_start + block_offset, buf);
    buf[byte_in_block] &= ~(1 << bit_offset);
    writeBlock(bitmap_block_start + block_offset, buf);
}

bool MiniFS::test_bit(int bitmap_block_start, int index)
{
    //index是inum，也可以是数据块号
    int byte_index = index / 8;
    int bit_offset = index % 8;
    int block_offset = byte_index / BLOCK_SIZE;
    int byte_in_block = byte_index % BLOCK_SIZE; 

    Byte buf[BLOCK_SIZE];
    readBlock(bitmap_block_start + block_offset, buf);
    return (buf[byte_in_block] & (1 << bit_offset)) != 0;
}

int MiniFS::find_free_bit(int bitmap_block_start, int total_bits, int min_allowed_index)
{
    // total_bits 指的是: 此位图管理的总项目数 (例如: INODE_NUM 或 DATA_ZONE_NUM)
    // min_allowed_index: 允许返回的最小索引号 (例如，对于 inode 可能是 1，对于 data block 可能是 0)

    // 计算位图占用了多少个磁盘块
    int bits_per_block = 8 * BLOCK_SIZE;
    int blocks_to_check = (total_bits + bits_per_block - 1) / bits_per_block;

    // 第一个for是块偏移量， 第二个for是块内字节偏移量
    for (int block_offset = 0; block_offset < blocks_to_check; block_offset++)
    {
        Byte buf[BLOCK_SIZE]; // 定义一个缓冲区，大小为一个磁盘块
        readBlock(bitmap_block_start + block_offset, buf); // 一次读取一个完整的磁盘块

        for (int byte_in_block = 0; byte_in_block < BLOCK_SIZE; byte_in_block++)
        {
            if (buf[byte_in_block] != 0xFF)
            { // 优化：如果这个字节全是1 (0xFF)，说明没有空闲位，跳过
                for (int bit_offset = 0; bit_offset < 8; bit_offset++)
                { // 检查这个字节中的每一位
                    if ((buf[byte_in_block] & (1 << bit_offset)) == 0)
                    { // 位运算检查该位是否为0 (空闲)
                        int index = (block_offset * bits_per_block) + (byte_in_block * 8) + bit_offset;
                        // ^ 注意这里计算 index 的小调整: (block_offset * bits_per_block) 更清晰
                        // 或者保持原来的: int index = (block_offset * BLOCK_SIZE + byte_in_block) * 8 + bit_offset;
                        // 两者在 BLOCK_SIZE 是字节数，bits_per_block = BLOCK_SIZE * 8 的前提下等价。
                        // 为保持和你原代码一致，我们用原来的：
                        // int index = (block_offset * BLOCK_SIZE + byte_in_block) * 8 + bit_offset;


                        // 确保索引在总位数范围内，并且大于等于允许的最小索引
                        if (index < total_bits && index >= min_allowed_index) {
                            return index;
                        }
                    }
                }
            }
        }
    }
    return -1; // 没有找到
}





// 分配一个空闲的数据块,返回值是绝对数据块号
int MiniFS::balloc()
{
    int free_block_index = find_free_bit(DATA_BITMAP_BLOCK_START, DATA_BLOCKS_NUM, 0);
    if (free_block_index == -1) {
        std::cerr << "错误：没有空闲的数据块" << std::endl;
        return -1;
    }
    
    set_bit(DATA_BITMAP_BLOCK_START, free_block_index);
    int absolute_block_num = DATA_START + free_block_index;
    
    // 清空新分配的数据块
    Byte zero_buf[BLOCK_SIZE] = {0};
    writeBlock(absolute_block_num, zero_buf);
    
    return absolute_block_num;
}

// 释放一个数据块
void MiniFS::bfree(int absolute_block_num)
{
    if (absolute_block_num < DATA_START || absolute_block_num >= BLOCK_COUNT) {
        std::cerr << "错误：非法的数据块号 " << absolute_block_num << std::endl;
        return;
    }
    
    int block_index = absolute_block_num - DATA_START;
    clear_bit(DATA_BITMAP_BLOCK_START, block_index);
}

/**
 * @brief 分配一个空闲的i-节点并初始化
 * 
 * 该函数在i-节点位图中查找第一个空闲位，标记为已使用，
 * 然后在磁盘上初始化对应的i-节点结构。
 * 
 * @param type 要分配的i-节点类型（文件或目录等），
 *             通常用宏定义如T_FILE、T_DIR等表示
 * 
 * @return int 成功时返回分配的i-节点号(inum)，
 *             失败时返回-1（如没有空闲i-节点）
 * 
 * @note 函数执行流程：
 * 1. 在位图中查找空闲i-节点
 * 2. 标记位图中对应位为已使用
 * 3. 计算i-节点在磁盘上的位置,算block和offset
 * 4. 读取对应块到缓冲区
 * 5. 将缓冲区对应位置转换为dinode指针并初始化
 * 6. 设置i-节点类型
 * 7. 将修改写回磁盘
 * 
 */
// 分配一个i-节点,给定类型，是普通文件还是目录，返回inum
int MiniFS::ialloc(int16_t type)
{
    int free_inode_index = find_free_bit(INODE_BITMAP_BLOCK_START, INODE_NUM, 1);
    if (free_inode_index == -1) {
        std::cerr << "错误：没有空闲的i-节点" << std::endl;
        return -1;
    }
    
    set_bit(INODE_BITMAP_BLOCK_START, free_inode_index);
    
    // 初始化新i-节点
    int block = INODE_START + (free_inode_index * INODE_SIZE) / BLOCK_SIZE;
    int offset = (free_inode_index * INODE_SIZE) % BLOCK_SIZE;
    
    Byte buf[BLOCK_SIZE];
    readBlock(block, buf);
    
    //用指针操纵这块指定区域
    dinode* inode = reinterpret_cast<dinode*>(buf + offset);
    std::memset(inode, 0, sizeof(dinode));
    inode->type = type;
    
    writeBlock(block, buf);
    
    return free_inode_index;
}

// 释放一个i-节点
void MiniFS::ifree(int inum)
{
    if (inum < 0 || inum >= INODE_NUM) {
        std::cerr << "错误：非法的i-节点号 " << inum << std::endl;
        return;
    }
    
    // 将i-节点标记为空闲
    int block = INODE_START + (inum * INODE_SIZE) / BLOCK_SIZE;
    int offset = (inum * INODE_SIZE) % BLOCK_SIZE;
    
    Byte buf[BLOCK_SIZE];
    readBlock(block, buf);
    
    dinode* inode = reinterpret_cast<dinode*>(buf + offset);
    inode->type = T_FREE;
    
    writeBlock(block, buf);
    
    // 更新位图
    clear_bit(INODE_BITMAP_BLOCK_START, inum);
}