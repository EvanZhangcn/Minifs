#include "minifs.hpp"

int main(int argc, char *argv[]) {
    MiniFS fs; // 创建文件系统对象

    const std::string fsfile = "my_unix_fs.dat";
    std::cout << "尝试加载文件系统镜像: " << fsfile << std::endl;
    MiniFS::FSStatus load_result = fs.loadFS(fsfile);
    
    if (load_result != MiniFS::FSStatus::OK) 
    {
        if (load_result == MiniFS::FSStatus::FAIL) {
            std::cout << "未检测到磁盘镜像，正在格式化新文件系统..." << std::endl;
        } else if (load_result == MiniFS::FSStatus::CORRUPT) {
            std::cout << "磁盘镜像已损坏或格式不匹配，正在重新格式化..." << std::endl;
        }
        
        std::cout << "开始格式化文件系统..." << std::endl;
        fs.format();
        std::cout << "文件系统格式化完成" << std::endl;
    } 
    else 
    {
        std::cout << "磁盘镜像加载成功！" << std::endl;
        
        std::cout << "开始文件系统一致性检查..." << std::endl;
        if (fs.checkFSConsistency() != 0) 
        {
            std::cout << "文件系统一致性检查失败，正在重新格式化..." << std::endl;
            fs.format();
        }
        else {
            std::cout << "文件系统一致性检查通过" << std::endl;
        }
    }
    
    std::cout << std::endl << "准备列出根目录内容..." << std::endl;
    fs.listRoot();
    
    // 在此处可以添加命令处理循环，实现用户交互
    
    std::cout << std::endl << "正在保存文件系统..." << std::endl;
    if (fs.saveFS(fsfile) == 0) {
        std::cout << "文件系统已成功保存到 " << fsfile << std::endl;
    } else {
        std::cout << "保存文件系统失败！" << std::endl;
    }
    
    std::cout << "程序结束" << std::endl;
    return 0;
}



