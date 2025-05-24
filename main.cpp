#include "fs_tests.hpp"
#include "shell_utils.hpp"
#include "minifs.hpp"
#include "encoding_utils.hpp"

// 在 main 函数中
int main(int argc, char *argv[]) {
    // 初始化控制台编码，解决中文乱码问题
    EncodingUtils::initConsoleEncoding();
    
    const std::string fsfile = "my_unix_fs.dat";
    MiniFS fs;
    
    std::cout << "========== MiniFS 文件系统启动 ==========" << std::endl;    // 尝试加载已有文件系统，如果失败则格式化
    std::cout << "尝试加载文件系统镜像: " << fsfile << std::endl;
    MiniFS::FSStatus load_result = fs.loadFS(fsfile);
    
    if (load_result != MiniFS::FSStatus::OK) {
        std::cout << "未检测到有效的文件系统镜像，正在格式化..." << std::endl;
        fs.format();
        std::cout << "文件系统格式化完成!" << std::endl;
    } else {
        std::cout << "文件系统镜像加载成功!" << std::endl;
    }
    
    // 检查是否有命令行参数来决定运行模式
    if (argc > 1 && std::string(argv[1]) == "--test") {
        // 测试模式：运行所有测试
        std::cout << "\n========== 测试模式 ==========" << std::endl;
        test_bitmap_operations(fs);
        test_directory_operations(fs);
        
        // 保存文件系统状态
        std::cout << "正在保存文件系统..." << std::endl;
        if (fs.saveFS(fsfile) == 0) {
            std::cout << "文件系统已成功保存到 " << fsfile << std::endl;
        } else {
            std::cout << "保存文件系统失败!" << std::endl;
        }
    } 
    else 
    {
        // 交互模式：启动命令行界面
        runInteractiveShell(fs, fsfile);
    }
    
    std::cout << "程序结束" << std::endl;
    return 0;
}