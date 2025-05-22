#include "minifs.hpp"

void test_bitmap_operations(MiniFS& fs) {
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


// 在 main 函数中
int main(int argc, char *argv[]) {
    const std::string fsfile = "my_unix_fs.dat";
    MiniFS fs;
    
    // 尝试加载已有文件系统，如果失败则格式化
    std::cout << "尝试加载文件系统镜像: " << fsfile << std::endl;    MiniFS::FSStatus load_result = fs.loadFS(fsfile);
      if (load_result != MiniFS::FSStatus::OK) {
        std::cout << "未检测到有效的文件系统镜像，正在格式化..." << std::endl;
        fs.format();
    } else {
        std::cout << "文件系统镜像加载成功!" << std::endl;
        // 跳过一致性检查，直接执行目录操作
        std::cout << "跳过文件系统一致性检查，直接测试目录操作..." << std::endl;
        // 可选: 如果需要强制格式化文件系统，取消下面的注释
        // fs.format();
        // std::cout << "文件系统已重新格式化" << std::endl;
    }
    
    // 运行测试
    // test_bitmap_operations(fs);
    test_directory_operations(fs);
    
    // 保存文件系统状态
    std::cout << "正在保存文件系统..." << std::endl;
    if (fs.saveFS(fsfile) == 0) {
        std::cout << "文件系统已成功保存到 " << fsfile << std::endl;
    } else {
        std::cout << "保存文件系统失败!" << std::endl;
    }
    
    return 0;
}
