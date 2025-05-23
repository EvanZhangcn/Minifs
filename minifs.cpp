#include "fs_tests.hpp"
#include "shell_utils.hpp"
#include "user.hpp"  // 在实现文件中引入user.hpp


// 构造函数 - 初始化虚拟磁盘
MiniFS::MiniFS() : userManager() { // 在构造函数初始化列表中初始化 userManager,这里因为没初始化一直报错，一定要初始化
    // 初始化文件描述符表
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        fd_table[i].is_used = false;
        fd_table[i].inum = -1;
        fd_table[i].position = 0;
        fd_table[i].mode = 0;
    }
    
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

// MiniFS 析构函数
MiniFS::~MiniFS() {
    // userManager 作为 MiniFS 的直接成员，其析构函数会自动调用，无需手动 delete
}

// 块读取方法
void MiniFS::readBlock(int blockNum, void* buf) 
{
    try {
        // 增加安全检查，确保不会越界访问
        if (blockNum < 0 || blockNum >= BLOCK_COUNT) {
            std::cerr << "错误: readBlock 尝试读取无效块号: " << blockNum 
                    << " (有效范围: 0-" << (BLOCK_COUNT-1) << ")" << std::endl;
            // 清零缓冲区而不是尝试读取无效内存
            std::memset(buf, 0, BLOCK_SIZE);
            return;
        }
        
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
        
        // 计算源地址并执行复制
        const void* src_addr = disk.data() + blockNum * BLOCK_SIZE;
        std::memcpy(buf, src_addr, BLOCK_SIZE);
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

/**
 * @brief 读取指定 i-节点号对应的 i-节点信息
 * 
 * @param inum      [输入] 要查询的 i-节点号，必须满足 0 < inum < INODE_NUM
 * @param node_out  [输出] 成功时存储读取到的 i-节点信息（dinode 结构体）
 * 
 * @return true     读取成功且 i-节点非空闲（node_out 包含有效数据）
 * @return false    读取失败（无效 inum/块号、i-节点空闲或磁盘读取错误）
 */
// 辅助函数：读取i-节点信息 (现在改为public)
bool MiniFS::_get_inode(int inum, dinode& node_out) {
    if (inum <= 0 || inum >= INODE_NUM) 
    { 
        // 0号i-节点通常不用，或作为NIL
        // std::cerr << "调试: _get_inode 无效的 i-节点号 " << inum << std::endl;
        return false;
    }
    int block = INODE_START + (inum * INODE_SIZE) / BLOCK_SIZE;
    int offset = (inum * INODE_SIZE) % BLOCK_SIZE;
    
    // 检查块号是否有效
    if (block < 0 || block >= BLOCK_COUNT) 
    {
        std::cerr << "错误: _get_inode 计算出的块号 " << block << " 无效 (i-节点 " << inum << ")" << std::endl;
        return false;
    }

    //采用buf，读出来block的数据到buf， 再用block + offset地址，访问出来的数据读到传进来的临时变量node_out
    Byte buf[BLOCK_SIZE];
    readBlock(block, buf); // readBlock 内部应有错误处理
    std::memcpy(&node_out, buf + offset, sizeof(dinode));

    if (node_out.type == T_FREE) {
        // std::cerr << "调试: _get_inode i-节点 " << inum << " 是空闲的." << std::endl;
        return false; 
    }
    return true;
}


/**
 * @brief 在指定目录中查找指定名称的目录条目
 * @param dir_inum  [输入] 要搜索的目录的i-节点号，必须是一个有效的目录i-节点
 * @param name      [输入] 要查找的条目名称，长度不应超过DIRSIZ-1
 * 
 * @return int      成功时返回找到的条目的i-节点号(>0)
 * @return INVALID_INUM_CONST (通常为0或-1) 表示未找到或出现错误，包括：
 *                 - 目录i-节点无效或不是目录类型
 *                 - 目录数据块无效
 *                 - 条目不存在
 * @note 名称比较是精确匹配，区分大小写
 */
int MiniFS::_lookup_in_directory(int dir_inum, const std::string& name) {
     //重点就是：遍历dir_node的目录条目，进行名称匹配：
     //   - 跳过无效条目（inum为0或INVALID_INUM_CONST）
     //   - 精确比较名称（区分大小写）

    dinode dir_node;
    if (!_get_inode(dir_inum, dir_node)) 
    {
        // _get_inode 已经输出了错误（如果存在）
        return INVALID_INUM_CONST;
    }
    //此时dir_node已经拿到了，进一步检测类型，地址，最后通过dir_node.addrs[0]访问数据块

    if (dir_node.type != T_DIR) 
    {
        std::cerr << "错误: i-节点 " << dir_inum << " 不是一个目录 (类型: " << dir_node.type << ")." << std::endl;
        return INVALID_INUM_CONST;
    }

    if (dir_node.addrs[0] == 0) 
    { // 没有分配数据块
        // std::cerr << "调试: 目录 " << dir_inum << " 没有分配数据块。" << std::endl;
        return INVALID_INUM_CONST;
    }
    // 检查数据块号是否有效
    if (dir_node.addrs[0] < DATA_START || dir_node.addrs[0] >= BLOCK_COUNT) 
    {
        std::cerr << "错误: 目录 " << dir_inum << " 的数据块号 " << dir_node.addrs[0] << " 无效。" << std::endl;
        return INVALID_INUM_CONST;
    }


    Byte block_buf[BLOCK_SIZE];
    readBlock(dir_node.addrs[0], block_buf);

    int num_entries_possible = BLOCK_SIZE / sizeof(dirent); // 一个块最多能放多少个
    int actual_entries = dir_node.size / sizeof(dirent); //实际放了多少个，检测下是否超出第一块
    if (actual_entries > num_entries_possible) 
    {
        std::cerr << "警告: 目录 " << dir_inum << " 大小 (" << dir_node.size << ") 超出单个块容量，仅检查第一块。" << std::endl;
        actual_entries = num_entries_possible;
    }
    
    dirent* entries = reinterpret_cast<dirent*>(block_buf);

    //遍历每一项，并比较name
    for (int i = 0; i < actual_entries; ++i) {
        // 确保比较的长度正确，并且 entries[i].name 是有效C字符串
        if (entries[i].inum != 0 && entries[i].inum != INVALID_INUM_CONST) 
        { // 假设0或INVALID_INUM表示空闲条目
             // strncmp 比较安全，防止dirent.name未正确null结尾
            if (strncmp(entries[i].name, name.c_str(), DIRSIZ -1) == 0 && name.length() == strlen(entries[i].name) ) 
            {
                 // Make sure entries[i].name is null-terminated for strlen
                 // A safer way might be if DIRSIZ guarantees space for null terminator
                 // and all names are stored null-terminated.
                 // For this example, direct strcmp is often used assuming valid names.
                 if (name == entries[i].name) 
                 { // Simpler if names are guaranteed well-formed
                    return entries[i].inum;
                 }
            }
        }
    }
    return INVALID_INUM_CONST; // 未找到
}

// 将路径字符串解析到i-节点号
//它接收一个字符串形式的路径(如 /home/user 或 docs/report.txt) 和  一个当前工作目录的 i-节点号(默认为根目录 1)
//拿到最终访问该文件/文件夹的inum号
int MiniFS::resolve_path_to_inum(const std::string& path, int base_inum) {
    std::string current_path_str = path;
    std::vector<std::string> components;//分割路径每两个/之间的内容, 这是个栈，存segment

    // 1. 规范化路径：移除末尾的一个或者多个'/' (除非路径就是 "/")
    // /home/user/ → /home/user
    // /a//b/// → /a//b（只移除末尾的斜杠）
    while (current_path_str.length() > 1 && current_path_str.back() == '/') 
    {
        current_path_str.pop_back();
    }

    //针对特殊情况：原始路径是 "///"，经过上面的while操作已经为空了，但是原始path不为空
    if (current_path_str.empty() && !path.empty() && path[0] == '/') 
    { 
        current_path_str = "/";
    }
    
    if (current_path_str.empty()) 
    { 
        // 如果原始路径为空，或处理后为空 (例如 "////" 且非根)
        return path[0] == '/' ? ROOT_INUM_CONST : base_inum; // "" -> base_inum, "/" -> ROOT
    }

    if (current_path_str == "/") {
        return ROOT_INUM_CONST;
    }

    // 2. 确定起始i-节点
    int current_inum;
    std::stringstream ss(current_path_str);
    std::string segment;//存每一部分的分段

    if (current_path_str[0] == '/') 
    {
        current_inum = ROOT_INUM_CONST;
        //ss将字符串转化为流，配合getline指定分隔符，产生segment
        std::getline(ss, segment, '/'); // 跳过第一个由'/'产生的空segment
    } 
    else 
    {
        current_inum = base_inum;
    }

    // 3. 分解路径组件
    while (std::getline(ss, segment, '/')) 
    {
        // /a//b
        if (!segment.empty()) 
        { 
            // 忽略因 "//" 产生的空组件
            components.push_back(segment);
        }
    }

    //对于命令 ls new_dir，  create 文件名
    // 情况：如果 components 为空且路径不是以 '/' 开头（即相对路径）
    if (components.empty() && current_path_str[0] != '/') 
    { 
        // 检查路径是否非空且不包含 '/'（即单级相对路径，如 "my_dir"）
        if (!current_path_str.empty() && current_path_str.find('/') == std::string::npos) {
            // 将整个路径字符串作为唯一component入栈
            components.push_back(current_path_str); 
        }
    }

    // 4. 逐级解析
    dinode current_node_obj; // 临时变量，用于临时存储i节点信息
    for (const std::string& comp : components) 
    {
        if (comp.empty()) continue; //防一下：是否有空格被入栈components里面了
        if (!_get_inode(current_inum, current_node_obj)) 
        {
            std::cerr << "路径解析错误: 无法读取 i-节点 " << current_inum << " (处理组件: '" << comp << "')" << std::endl;
            return INVALID_INUM_CONST;
        }

        //检验类型是否为directory
        if (current_node_obj.type != T_DIR) 
        {
            std::cerr << "路径解析错误: i-节点 " << current_inum << " 不是目录 (组件: '" << comp << "')" << std::endl;
            return INVALID_INUM_CONST;
        }


        if (comp == ".") 
        {
            // 当前目录，i-节点号不变
            continue;
        } 
        else if (comp == "..") 
        {
            // 父目录
            // 查找 ".." 条目，它应该由 format 或 mkdir 创建
            // 根目录的 ".." 指向根目录自身
            int parent_inum = _lookup_in_directory(current_inum, "..");
            //返回值有两种，parent_inum == INVALID_INUM_CONST 或者 正确的inum值
            if (parent_inum == INVALID_INUM_CONST) {
                std::cerr << "路径解析错误: 目录 " << current_inum << " 中未找到 '..' 条目。" << std::endl;
                return INVALID_INUM_CONST;
            }
            current_inum = parent_inum;
        } 
        else 
        {
            // 普通目录/文件组件
            int found_inum = _lookup_in_directory(current_inum, comp);
            if (found_inum == INVALID_INUM_CONST) {
                // std::cerr << "路径解析错误: 在目录 " << current_inum << " 中未找到 '" << comp << "'." << std::endl;
                return INVALID_INUM_CONST; // Not found, return error
            }
            current_inum = found_inum;
        }
    }

    // 最终的current_inum是结果，再验证一次它是否有效（非空闲）
    if (current_inum != INVALID_INUM_CONST) {
        if (!_get_inode(current_inum, current_node_obj)) {
            // 可能指向一个已被释放的i-节点
            return INVALID_INUM_CONST;
        }
    }
    return current_inum;
}

/**
 * @brief 在指定路径创建一个新的普通文件。
 * @return int 成功时返回0，失败时返回负数错误码。
 * -1: 通用错误
 * INVALID_INUM_CONST: 路径无效或父目录不存在
 * -2: 路径中某一部分不是目录
 * -3: 文件已存在
 * -4: i-节点分配失败
 * -5: 父目录写入失败
 */
int MiniFS::create(int parent_dir_inum, const char* name)
{    
    // 检查文件名长度
    if (strlen(name) >= DIRSIZ) {
        std::cerr << "错误: 文件名称过长 (最大长度: " << DIRSIZ-1 << " 字符)" << std::endl;
        return INVALID_INUM_CONST;
    }

    // 1. 读取父目录i-节点
    int parent_block = INODE_START + (parent_dir_inum * INODE_SIZE) / BLOCK_SIZE;
    int parent_offset = (parent_dir_inum * INODE_SIZE) % BLOCK_SIZE;
    
    Byte buf[BLOCK_SIZE];
    readBlock(parent_block, buf);
    dinode parent_inode;
    std::memcpy(&parent_inode, buf + parent_offset, sizeof(dinode));
    
    // 确认父节点是一个目录
    if (parent_inode.type != T_DIR) {
        std::cerr << "错误: 父i-节点不是目录类型 (type = " << parent_inode.type << ")" << std::endl;
        return INVALID_INUM_CONST;
    }
    
    // 2. 检查同名冲突
    int entries_count = parent_inode.size / sizeof(dirent);
    dirent entries[BLOCK_SIZE / sizeof(dirent)];
    readBlock(parent_inode.addrs[0], entries);  // 简化: 假设目录项都在第一个数据块
    
    for (int i = 0; i < entries_count; i++) {
        if (std::strcmp(entries[i].name, name) == 0) {
            std::cerr << "错误: 父目录中已存在同名项 '" << name << "'" << std::endl;
            return INVALID_INUM_CONST;
        }
    }
    
    // 3. 分配新文件的i-节点
    int file_inum = ialloc(T_FILE);
    if (file_inum == INVALID_INUM_CONST) {
        std::cerr << "错误: 无法分配i-节点" << std::endl;
        return INVALID_INUM_CONST;
    }
    
    // 4. 分配新文件的第一个数据块
    int file_data_block = balloc();
    if (file_data_block == -1) {
        std::cerr << "错误: 无法分配数据块" << std::endl;
        ifree(file_inum); // 释放之前分配的i-节点
        return INVALID_INUM_CONST;
    }
    
    // 5. 初始化新文件的i-节点
    int file_block = INODE_START + (file_inum * INODE_SIZE) / BLOCK_SIZE;
    int file_offset = (file_inum * INODE_SIZE) % BLOCK_SIZE;
    
    readBlock(file_block, buf);
    dinode* file_inode = reinterpret_cast<dinode*>(buf + file_offset);
    
    file_inode->type = T_FILE;
    file_inode->size = 0;  // 新创建的文件大小为0
    file_inode->nlink = 1; // 只有父目录的一个链接
    file_inode->addrs[0] = file_data_block;
    writeBlock(file_block, buf);
    
    // 6. 初始化文件数据块（全为0）
    // 实际上 balloc 已经做了数据块清零操作，这里可以不需要
    // Byte zero_buf[BLOCK_SIZE] = {0};
    // writeBlock(file_data_block, zero_buf);
    
    // 7. 在父目录中添加新文件条目
    dirent new_entry;
    new_entry.inum = file_inum;
    std::strcpy(new_entry.name, name);//名字赋值
    entries[entries_count] = new_entry; // 这行是必要的，添加新目录项
    
    // 8. 写回更新后的父目录数据
    writeBlock(parent_inode.addrs[0], entries);
    
    // 9. 写回更新后的父目录i-节点（只更新大小，不增加链接数）
    parent_inode.size += sizeof(dirent);  // 增加父目录大小
    std::memcpy(buf + parent_offset, &parent_inode, sizeof(dinode));
    writeBlock(parent_block, buf);
    std::cout << "成功创建文件: " << name << " (inum: " << file_inum << ")" << std::endl;
    return file_inum;
}



// 打开文件函数
// parent_dir_inum: 父目录i-节点号
// name: 文件名
// flags: 打开模式，支持 O_RDONLY, O_WRONLY, O_RDWR, O_CREATE
// 返回值: 成功返回文件描述符，失败返回-1
int MiniFS::open(int parent_dir_inum, const char* name, int flags) 
{
    std::cout << "尝试打开文件: " << name << " (parent inum: " << parent_dir_inum << ")" << std::endl;

    // 检查文件名长度
    if (strlen(name) >= DIRSIZ) {
        std::cerr << "错误: 文件名称过长 (最大长度: " << DIRSIZ-1 << " 字符)" << std::endl;
        return -1;
    }

    // 1. 读取父目录i-节点
    int parent_block = INODE_START + (parent_dir_inum * INODE_SIZE) / BLOCK_SIZE;
    int parent_offset = (parent_dir_inum * INODE_SIZE) % BLOCK_SIZE;
    
    Byte buf[BLOCK_SIZE];
    readBlock(parent_block, buf);
    dinode parent_inode;
    std::memcpy(&parent_inode, buf + parent_offset, sizeof(dinode));
    
    // 确认父节点是一个目录
    if (parent_inode.type != T_DIR) {
        std::cerr << "错误: 父i-节点不是目录类型 (type = " << parent_inode.type << ")" << std::endl;
        return -1;
    }
    
    // 2. 在父目录中查找文件
    int entries_count = parent_inode.size / sizeof(dirent);
    dirent entries[BLOCK_SIZE / sizeof(dirent)];
    readBlock(parent_inode.addrs[0], entries);  // 简化: 假设目录项都在第一个数据块
    
    int file_inum = -1;
    for (int i = 0; i < entries_count; i++) {
        if (std::strcmp(entries[i].name, name) == 0) {
            file_inum = entries[i].inum;
            break;
        }
    }
    
    // 3. 如果文件不存在且没有设置O_CREATE标志，则返回错误
    if (file_inum == -1) {
        if (!(flags & O_CREATE)) {
            std::cerr << "错误: 文件不存在且未设置创建标志" << std::endl;
            return -1;
        } else 
        {
            // 创建文件
            file_inum = create(parent_dir_inum, name);
            if (file_inum == INVALID_INUM_CONST) {
                std::cerr << "错误: 无法创建文件" << std::endl;
                return -1;
            }
        }
    }
    
    // 4. 检查文件类型
    int file_block = INODE_START + (file_inum * INODE_SIZE) / BLOCK_SIZE;
    int file_offset = (file_inum * INODE_SIZE) % BLOCK_SIZE;
    
    readBlock(file_block, buf);
    dinode file_inode;
    std::memcpy(&file_inode, buf + file_offset, sizeof(dinode));
    
    if (file_inode.type != T_FILE) {
        std::cerr << "错误: 不是一个文件类型 (type = " << file_inode.type << ")" << std::endl;
        return -1;
    }
    
    // 5. 分配文件描述符
    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!fd_table[i].is_used) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) {
        std::cerr << "错误: 文件描述符表已满" << std::endl;
        return -1;
    }
    
    // 6. 设置文件描述符
    fd_table[fd].inum = file_inum;
    fd_table[fd].mode = flags & (O_RDONLY | O_WRONLY | O_RDWR); // 仅保留读写模式位，这里顺带保存了O_CREATE位
    fd_table[fd].position = 0; // 从文件开始处读写
    fd_table[fd].is_used = true;
    
    std::cout << "成功打开文件: " << name << " (fd: " << fd << ", inum: " << file_inum << ")" << std::endl;
    return fd;
}

// 关闭文件函数
// fd: 要关闭的文件描述符
// 返回值: 成功返回0，失败返回-1
int MiniFS::close(int fd)
{
    // 检查文件描述符是否有效
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        std::cerr << "错误: 无效的文件描述符 " << fd << std::endl;
        return -1;
    }
    
    // 检查文件描述符是否在使用中
    if (!fd_table[fd].is_used) {
        std::cerr << "错误: 文件描述符 " << fd << " 未使用" << std::endl;
        return -1;
    }
    
    // 关闭文件（标记为未使用）
    fd_table[fd].is_used = false;
    std::cout << "成功关闭文件描述符 " << fd << std::endl;
    
    return 0;
}

// 读取文件函数
// fd: 文件描述符
// buf: 读取数据的缓冲区
// count: 要读取的最大字节数
// 返回值: 成功返回实际读取的字节数，失败返回-1
//拿到文件描述符，得到关联节点inum，就有了inum->address
int MiniFS::read(int fd, void* buf, int count)
{
    // 检查文件描述符是否有效
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        std::cerr << "错误: 无效的文件描述符 " << fd << std::endl;
        return -1;
    }
    
    // 检查文件描述符是否在使用中
    if (!fd_table[fd].is_used) {
        std::cerr << "错误: 文件描述符 " << fd << " 未使用" << std::endl;
        return -1;
    }
    
    // 检查是否有读权限
    if (!(fd_table[fd].mode & (O_RDONLY | O_RDWR))) {
        std::cerr << "错误: 文件描述符 " << fd << " 没有读权限" << std::endl;
        return -1;
    }
    
    // 获取关联的i-节点
    int inum = fd_table[fd].inum;
    int inode_block = INODE_START + (inum * INODE_SIZE) / BLOCK_SIZE;
    int inode_offset = (inum * INODE_SIZE) % BLOCK_SIZE;
    
    Byte block_buf[BLOCK_SIZE];
    readBlock(inode_block, block_buf);
    dinode file_inode;
    std::memcpy(&file_inode, block_buf + inode_offset, sizeof(dinode));
    
    // 检查文件大小
    int file_size = file_inode.size;
    
    // 修改：始终从文件开头读取，但仍保留文件位置用于写操作
    // 原来是：int bytes_available = file_size - fd_table[fd].position;
    // 修改为总是从文件开头读取指定数量的字节
    int bytes_available = file_size; // 从文件开头读取
    
    if (bytes_available <= 0) 
    {
        // 文件为空
        return 0;
    }
    
    // 确定要读取的字节数
    int bytes_to_read = std::min(bytes_available, count);
    int bytes_read = 0;
    
    // 读取数据,用字符指针逐个读取
    char* dest_buf = static_cast<char*>(buf);
    
    // 修改：总是从文件开头读取，不使用文件位置
    // int curr_pos = fd_table[fd].position;
    int curr_pos = 0; // 从文件开头开始
    
    while (bytes_to_read > 0) {
        // 计算当前位置对应的数据块索引和偏移量
        int block_index = curr_pos / BLOCK_SIZE;
        int block_offset = curr_pos % BLOCK_SIZE;
        
        // 确保数据块索引合法
        if (block_index >= 8) {
            std::cerr << "错误: 文件读取超出支持的最大文件大小" << std::endl;
            break;
        }
        
        // 获取数据块
        int data_block_num = file_inode.addrs[block_index];
        if (data_block_num == 0) {
            // 没有分配的数据块，可能是文件有空洞
            break;
        }
        
        // 读取数据块
        Byte data_buf[BLOCK_SIZE];
        readBlock(data_block_num, data_buf);
        
        // 计算在当前块中可以读取的字节数
        int block_bytes = std::min(bytes_to_read, BLOCK_SIZE - block_offset);
        
        // 复制数据
        std::memcpy(dest_buf + bytes_read, data_buf + block_offset, block_bytes);
        
        // 更新计数和位置
        bytes_read += block_bytes;
        bytes_to_read -= block_bytes;
        curr_pos += block_bytes;
    }
    
    // 修改：不更新文件位置，position仅用于写入操作
    // fd_table[fd].position += bytes_read;
    
    std::cout << "成功从文件描述符 " << fd << " 读取 " << bytes_read << " 字节" << std::endl;
    //返回读取的字节数
    return bytes_read;
}

// 写入文件函数
// fd: 文件描述符
// buf: 要写入的数据
// count: 要写入的字节数
// 返回值: 成功返回实际写入的字节数，失败返回-1
int MiniFS::write(int fd, const void* buf, int count)
{
    // 检查文件描述符是否有效
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        std::cerr << "错误: 无效的文件描述符 " << fd << std::endl;
        return -1;
    }
    
    // 检查文件描述符是否在使用中
    if (!fd_table[fd].is_used) {
        std::cerr << "错误: 文件描述符 " << fd << " 未使用" << std::endl;
        return -1;
    }
    
    // 检查是否有写权限
    if (!(fd_table[fd].mode & (O_WRONLY | O_RDWR))) {
        std::cerr << "错误: 文件描述符 " << fd << " 没有写权限" << std::endl;
        return -1;
    }
    
    // 获取关联的i-节点,拿到inum，下一步定位当前数据块和块偏移
    int inum = fd_table[fd].inum;
    int inode_block = INODE_START + (inum * INODE_SIZE) / BLOCK_SIZE;
    int inode_offset = (inum * INODE_SIZE) % BLOCK_SIZE;
    
    Byte block_buf[BLOCK_SIZE];
    readBlock(inode_block, block_buf);
    dinode file_inode;
    std::memcpy(&file_inode, block_buf + inode_offset, sizeof(dinode));
    
    // 如果要写入的数据超出文件大小，可能需要扩展文件
    int target_size = fd_table[fd].position + count;
    int needed_blocks = (target_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    if (needed_blocks > 8) {
        std::cerr << "错误: 所需的块数 " << needed_blocks << " 超出了支持的最大值 8" << std::endl;
        // 只写入能够容纳的数据
        needed_blocks = 8;
        count = needed_blocks * BLOCK_SIZE - fd_table[fd].position;
    }
    
    // 分配必要的数据块
    int current_blocks = (file_inode.size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (int i = current_blocks; i < needed_blocks; i++) {
        int new_block = balloc();
        if (new_block == -1) {
            std::cerr << "错误: 无法分配数据块，磁盘空间不足" << std::endl;
            // 只写入已分配的块
            needed_blocks = i;
            count = (i * BLOCK_SIZE) - fd_table[fd].position;
            if (count <= 0) {
                return 0; // 无法写入任何数据
            }
            break;
        }
        file_inode.addrs[i] = new_block;
    }
    
    // 开始写入数据
    int bytes_written = 0;
    int curr_pos = fd_table[fd].position;
    const char* src_buf = static_cast<const char*>(buf);
    
    while (bytes_written < count) {
        // 计算当前位置对应的数据块索引和偏移量
        int block_index = curr_pos / BLOCK_SIZE;
        int block_offset = curr_pos % BLOCK_SIZE;
        
        // 获取数据块
        int data_block_num = file_inode.addrs[block_index];
        
        // 读取当前块内容
        Byte data_buf[BLOCK_SIZE];
        readBlock(data_block_num, data_buf);
        
        // 计算在当前块中可以写入的字节数
        int block_bytes = std::min(count - bytes_written, BLOCK_SIZE - block_offset);
        
        // 复制数据
        std::memcpy(data_buf + block_offset, src_buf + bytes_written, block_bytes);
        
        // 写回数据块
        writeBlock(data_block_num, data_buf);
        
        // 更新计数和位置
        bytes_written += block_bytes;
        curr_pos += block_bytes;
    }
    
    // 更新文件位置
    fd_table[fd].position += bytes_written;
    
    // 更新文件大小
    if (target_size > file_inode.size) {
        file_inode.size = target_size;
    }
    
    // 更新i-节点
    std::memcpy(block_buf + inode_offset, &file_inode, sizeof(dinode));
    writeBlock(inode_block, block_buf);
    
    std::cout << "成功向文件描述符 " << fd << " 写入 " << bytes_written << " 字节" << std::endl;
    return bytes_written;
}

// 删除目录函数
// parent_dir_inum: 父目录的i-节点号
// name: 要删除的目录名
// 返回值: 成功返回0，失败返回-1
int MiniFS::rmdir(int parent_dir_inum, const char* name)
{
    std::cout << "开始删除目录: " << name << " (parent inum: " << parent_dir_inum << ")" << std::endl;
    
    if (strlen(name) >= DIRSIZ) {
        std::cerr << "错误: 目录名称过长 (最大长度: " << DIRSIZ-1 << " 字符)" << std::endl;
        return -1;
    }

    // 1. 读取父目录i-节点
    int parent_block = INODE_START + (parent_dir_inum * INODE_SIZE) / BLOCK_SIZE;
    int parent_offset = (parent_dir_inum * INODE_SIZE) % BLOCK_SIZE;
    
    Byte buf[BLOCK_SIZE];
    readBlock(parent_block, buf);
    dinode parent_inode;
    std::memcpy(&parent_inode, buf + parent_offset, sizeof(dinode));
    
    // 确认父节点是一个目录
    if (parent_inode.type != T_DIR) {
        std::cerr << "错误: 父i-节点不是目录类型 (type = " << parent_inode.type << ")" << std::endl;
        return -1;
    }
    
    // 2. 在父目录中查找目标目录的i-节点号
    int entries_count = parent_inode.size / sizeof(dirent);
    dirent entries[BLOCK_SIZE / sizeof(dirent)];
    readBlock(parent_inode.addrs[0], entries);
    
    int target_index = -1;
    int target_inum = -1;
    
    for (int i = 0; i < entries_count; i++) {
        if (std::strcmp(entries[i].name, name) == 0) {
            target_index = i;
            target_inum = entries[i].inum;
            break;
        }
    }
    
    if (target_index == -1 || target_inum == -1) {
        std::cerr << "错误: 目录 '" << name << "' 不存在" << std::endl;
        return -1;
    }
    
    // 3. 读取要删除的目录的i-节点
    int target_block = INODE_START + (target_inum * INODE_SIZE) / BLOCK_SIZE;
    int target_offset = (target_inum * INODE_SIZE) % BLOCK_SIZE;
    
    readBlock(target_block, buf);
    dinode target_inode;
    std::memcpy(&target_inode, buf + target_offset, sizeof(dinode));
    
    // 确认目标是一个目录
    if (target_inode.type != T_DIR) {
        std::cerr << "错误: 目标 '" << name << "' 不是一个目录 (type = " << target_inode.type << ")" << std::endl;
        return -1;
    }
    
    // 4. 检查目录是否为空（只包含 . 和 ..）
    if (target_inode.size > 2 * sizeof(dirent)) {
        dirent dir_entries[BLOCK_SIZE / sizeof(dirent)];
        readBlock(target_inode.addrs[0], dir_entries);
        int dir_entries_count = target_inode.size / sizeof(dirent);
        
        // 只有 . 和 .. 时才能删除
        if (dir_entries_count > 2) {
            std::cerr << "错误: 目录 '" << name << "' 不为空，包含 " << dir_entries_count - 2 << " 个项目" << std::endl;
            return -1;
        }
    }
    
    // 5. 从父目录中移除该条目 (通过将最后一个条目移到被删除条目的位置)
    if (target_index < entries_count - 1) {
        entries[target_index] = entries[entries_count - 1];
    }
    parent_inode.size -= sizeof(dirent);
    parent_inode.nlink--; // 减少父目录的链接数
    
    // 6. 释放目录的数据块
    for (int i = 0; i < 8; i++) {
        if (target_inode.addrs[i] != 0) {
            bfree(target_inode.addrs[i]);
        }
    }
    
    // 7. 释放目录的i-节点
    ifree(target_inum);
    
    // 8. 更新父目录的i-节点和数据
    writeBlock(parent_inode.addrs[0], entries);
    std::memcpy(buf + parent_offset, &parent_inode, sizeof(dinode));
    writeBlock(parent_block, buf);
    
    std::cout << "成功删除目录: " << name << std::endl;
    return 0;
}

// 删除文件函数
// parent_dir_inum: 父目录的i-节点号
// name: 要删除的文件名
// 返回值: 成功返回0，失败返回-1
int MiniFS::rm(int parent_dir_inum, const char* name)
{
    std::cout << "开始删除文件: " << name << " (parent inum: " << parent_dir_inum << ")" << std::endl;
    
    if (strlen(name) >= DIRSIZ) {
        std::cerr << "错误: 文件名称过长 (最大长度: " << DIRSIZ-1 << " 字符)" << std::endl;
        return -1;
    }

    // 1. 读取父目录i-节点
    int parent_block = INODE_START + (parent_dir_inum * INODE_SIZE) / BLOCK_SIZE;
    int parent_offset = (parent_dir_inum * INODE_SIZE) % BLOCK_SIZE;
    
    Byte buf[BLOCK_SIZE];
    readBlock(parent_block, buf);
    dinode parent_inode;
    std::memcpy(&parent_inode, buf + parent_offset, sizeof(dinode));
    
    // 确认父节点是一个目录
    if (parent_inode.type != T_DIR) {
        std::cerr << "错误: 父i-节点不是目录类型 (type = " << parent_inode.type << ")" << std::endl;
        return -1;
    }
    
    // 2. 在父目录中查找目标文件的i-节点号
    int entries_count = parent_inode.size / sizeof(dirent);
    dirent entries[BLOCK_SIZE / sizeof(dirent)];
    readBlock(parent_inode.addrs[0], entries);
    
    int target_index = -1;
    int target_inum = -1;
    
    for (int i = 0; i < entries_count; i++) {
        if (std::strcmp(entries[i].name, name) == 0) {
            target_index = i;
            target_inum = entries[i].inum;
            break;
        }
    }
    
    if (target_index == -1 || target_inum == -1) {
        std::cerr << "错误: 文件 '" << name << "' 不存在" << std::endl;
        return -1;
    }
    
    // 3. 读取要删除的文件i-节点
    int target_block = INODE_START + (target_inum * INODE_SIZE) / BLOCK_SIZE;
    int target_offset = (target_inum * INODE_SIZE) % BLOCK_SIZE;
    
    readBlock(target_block, buf);
    dinode target_inode;
    std::memcpy(&target_inode, buf + target_offset, sizeof(dinode));
    
    // 确认目标是一个文件
    if (target_inode.type != T_FILE) {
        std::cerr << "错误: 目标 '" << name << "' 不是一个文件 (type = " << target_inode.type << ")" << std::endl;
        return -1;
    }
    
    // 4. 检查该文件是否正在被使用
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (fd_table[i].is_used && fd_table[i].inum == target_inum) {
            std::cerr << "错误: 文件 '" << name << "' 正在被文件描述符 " << i << " 使用中" << std::endl;
            return -1;
        }
    }
    
    // 5. 从父目录中移除该文件条目
    if (target_index < entries_count - 1) {
        entries[target_index] = entries[entries_count - 1];
    }
    parent_inode.size -= sizeof(dirent);
    
    // 6. 释放文件的数据块
    for (int i = 0; i < 8; i++) {
        if (target_inode.addrs[i] != 0) {
            bfree(target_inode.addrs[i]);
        }
    }
    
    // 7. 释放文件的i-节点
    ifree(target_inum);
    
    // 8. 更新父目录的i-节点和数据
    writeBlock(parent_inode.addrs[0], entries);
    std::memcpy(buf + parent_offset, &parent_inode, sizeof(dinode));
    writeBlock(parent_block, buf);
    
    std::cout << "成功删除文件: " << name << std::endl;
    return 0;
}

// 登录函数
bool MiniFS::login(const std::string& username, const std::string& password) {
    return userManager.login(username, password); 
}

// 登出函数
bool MiniFS::logout() {
    return userManager.logout();
}

// 添加用户
bool MiniFS::addUser(const std::string& username, const std::string& password, int uid, int gid) {
    return userManager.addUser(username, password, uid, gid);
}

// 列出用户
void MiniFS::listUsers() {
    userManager.listUsers();
}

// 检查是否登录
bool MiniFS::isLoggedIn() const {
    return userManager.isLoggedIn();
}

// 获取当前用户
const User& MiniFS::getCurrentUser() const {
    return userManager.getCurrentUser();
}

// 保存用户数据
bool MiniFS::saveUserData() {
    return userManager.saveUsersToFS(this); // 将 this 指针传递给 UserManager
}

// 加载用户数据
bool MiniFS::loadUserData() {
    return userManager.loadUsersFromFS(this); // 将 this 指针传递给 UserManager
}

// 格式化时保留用户
void MiniFS::formatWithUserPreservation() {
    // 1. 保存当前用户数据
    std::string users_data_str = userManager.getUsersDataString();

    // 2. 执行标准格式化
    format(); // 这会清除所有数据，包括用户数据文件（如果存在）

    // 3. 恢复用户数据
    if (!userManager.parseUsersDataString(users_data_str)) {
        std::cerr << "警告: 格式化后恢复用户数据失败。正在使用默认用户配置。" << std::endl;
        // 如果解析失败，确保至少有 root 用户
        userManager.addUser("root", "root", 0, 0);
    }
    // 重新保存用户数据到新的文件系统结构中
    saveUserData();
}

// 清空用户数据（用于测试）
void MiniFS::clearUserData() {
    userManager.clearUsers();
}
