# MiniFS 发布说明

## 静态编译版本特性

✅ **静态链接编译** - 包含所有必要的运行时库  
✅ **跨系统兼容** - 可在不同 Windows 系统上直接运行  
✅ **无依赖部署** - 不需要安装额外的运行时环境  
✅ **优化编译** - 使用 -O2 优化，提升运行性能  

## 文件清单

### 核心文件
- `minifs.exe` - 主程序（静态编译，约 2.8MB）
- `my_unix_fs.dat` - 文件系统数据文件（运行时自动生成）

### 开发文件
- `build_static.bat` - 静态编译脚本
- `Makefile` - 跨平台编译配置
- `README.md` - 完整使用说明

### 源代码文件
- `main.cpp` - 程序入口
- `minifs.cpp/hpp` - 文件系统实现
- `shell_utils.cpp/hpp` - Shell 界面
- `fs_tests.cpp/hpp` - 测试代码
- `user.cpp/hpp` - 用户管理

## 编译信息

```
编译器: g++
标准: C++11
优化: -O2
链接: 静态链接 (-static -static-libgcc -static-libstdc++)
文件大小: ~2.8MB
```

## 使用方法

### 1. 直接运行（推荐）
```bash
# 使用启动脚本（自动解决中文乱码）
启动MiniFS.bat

# 测试模式
测试MiniFS.bat
```

### 2. 手动运行
```bash
# 如果出现中文乱码，先设置编码
chcp 65001
minifs.exe

# 测试模式
minifs.exe --test
```

### 3. 重新编译
```bash
# Windows
build_static.bat

# 跨平台
make static
```

## 中文乱码解决方案

本版本已内置中文乱码解决方案：

1. **程序内置编码处理**：
   - 启动时自动设置控制台为UTF-8
   - 使用Windows API设置代码页

2. **启动脚本**：
   - `启动MiniFS.bat` - 自动设置UTF-8编码后启动
   - `测试MiniFS.bat` - 自动设置UTF-8编码后运行测试

3. **编译脚本**：
   - `build_static.bat` - 也会设置UTF-8编码

### 技术实现
- 使用 `SetConsoleCP(CP_UTF8)` 和 `SetConsoleOutputCP(CP_UTF8)`
- 批处理脚本中使用 `chcp 65001`
- 静态编译确保在所有Windows系统上正常运行

## 系统要求

- **Windows**: Windows 7 或更高版本
- **内存**: 最少 4MB 可用内存
- **磁盘**: 约 10MB 可用空间

## 分发建议

### 最小分发包
只需要以下文件：
- `minifs.exe`
- `README.md`

### 完整开发包
包含所有源代码和编译脚本，用户可以自行修改和编译。

## 技术说明

### 静态编译的优势
1. **部署简便**: 一个 exe 文件即可运行
2. **兼容性好**: 不依赖特定版本的运行时库
3. **性能稳定**: 避免了动态链接的开销

### 文件系统特性
- 块大小: 512 字节
- 总容量: 512KB (1024 块)
- 支持文件和目录操作
- 数据持久化存储

---

**编译时间**: 2025年5月23日  
**版本**: 静态编译版 v1.0
