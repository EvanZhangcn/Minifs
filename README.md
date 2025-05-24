
# MiniFS - 最小UNIX文件系统模拟程序

一个基于C++实现的UNIX风格文件系统模拟器，支持目录操作、文件读写、用户管理等完整功能。

## 🚀 快速开始

### 直接运行（推荐）

如果您只是想体验 MiniFS，可以直接运行预编译的可执行文件：

**方式一：使用启动脚本（推荐，解决中文乱码）**
```bash
# 双击运行启动脚本，自动设置UTF-8编码
启动MiniFS.bat    # 交互模式
测试MiniFS.bat    # 测试模式
```

**方式二：直接运行**
```bash
# 直接运行（可能在某些系统上有中文乱码）
./minifs.exe      # 交互模式
./minifs.exe --test  # 测试模式
```

### 自行编译

#### Windows (推荐方法)

1. **使用批处理脚本（最简单）**：
   ```bash
   build_static.bat
   ```

2. **使用 VS Code**：
   - 按 `Ctrl+Shift+P`
   - 输入 "Tasks: Run Task"
   - 选择 "静态编译 MiniFS (推荐)"

3. **使用 Makefile**：
   ```bash
   make static
   ```

## 💡 中文乱码解决方案

如果在其他电脑上运行时出现中文乱码，请：

1. **使用提供的启动脚本**：
   - `启动MiniFS.bat` - 交互模式
   - `测试MiniFS.bat` - 测试模式
   
2. **手动设置编码**：
   ```bash
   chcp 65001
   minifs.exe
   ```

3. **程序已内置编码处理**：
   - 程序启动时会自动设置控制台为UTF-8编码
   - 如果仍有问题，请使用启动脚本

## 项目结构

```
├── minifs.exe          - 静态编译的可执行文件 ⭐
├── 启动MiniFS.bat      - 启动脚本（解决中文乱码）⭐
├── 测试MiniFS.bat      - 测试脚本（解决中文乱码）⭐
├── build_static.bat   - Windows 静态编译脚本 ⭐
├── encoding_utils.hpp - 编码处理工具 ⭐
├── main.cpp           - 主程序入口
├── minifs.hpp         - 文件系统核心头文件
├── minifs.cpp         - 文件系统核心实现
├── shell_utils.hpp    - 交互式Shell工具头文件
├── shell_utils.cpp    - 交互式Shell实现
├── user.hpp           - 用户管理系统头文件
├── user.cpp           - 用户管理系统实现
├── fs_tests.hpp       - 测试模块头文件
├── fs_tests.cpp       - 文件系统测试用例
├── Makefile          - 跨平台编译配置
├── .vscode/tasks.json - VS Code编译任务配置
├── README.md          - 项目说明文档
├── RELEASE.md         - 发布说明
└── my_unix_fs.dat     - 文件系统数据文件（运行时生成）
```

## 编译方法

⚠️ **重要提示：使用VS Code编译时，请选择 "编译 minifs (C++)" 任务，而不是默认的g++任务！**

### 方法一：使用VS Code（推荐）

1. 在VS Code中打开项目文件夹
2. 按 `Ctrl+Shift+P` 打开命令面板
3. 输入 "Tasks: Run Task"
4. **选择 "编译 minifs (C++)"** （不要选择默认的g++任务）

### 方法二：手动编译

在命令行中执行：

```bash
g++ -g minifs.cpp main.cpp fs_tests.cpp shell_utils.cpp -o minifs.exe
```

## 运行方法

### 交互模式（推荐）

```bash
./minifs.exe
```

### 测试模式

```bash
./minifs.exe --test
```

## 用户登录

### 默认管理员账户

- **用户名：`root`**
- **密码：`root`**

### 登录步骤

1. 启动程序后，输入以下命令进行登录：
   ```
   login root
   ```
2. 系统会提示输入密码，输入：`root`
3. 登录成功后，命令提示符会显示当前用户名

## 命令帮助

登录后输入 `help` 命令查看所有可用的操作命令。

主要命令包括：

### 目录操作

- `ls [路径]` - 列出目录内容
- `mkdir <路径>` - 创建目录
- `rmdir <路径>` - 删除空目录
- `cd <路径>` - 切换目录

### 文件操作

- `create <文件名>` - 创建文件
- `rm <文件名>` - 删除文件
- `open <文件名> <模式>` - 打开文件（模式：r/w/rw/c）
- `read <fd> <字节数>` - 读取文件
- `write <fd> <内容>` - 写入文件
- `close <fd>` - 关闭文件

### 用户管理

- `login <用户名>` - 用户登录
- `logout` - 用户登出
- `users` - 列出所有用户
- `whoami` - 显示当前用户
- `useradd <用户名> <UID> <GID>` - 添加用户

### 系统操作

- `status` - 显示文件系统状态
- `save` - 保存文件系统到磁盘
- `format` - 格式化文件系统
- `exit` - 退出程序

## 功能特性

### 核心功能

- ✅ 虚拟磁盘管理（1024个512字节的块）
- ✅ i-节点系统（128个i-节点）
- ✅ 位图管理（i-节点位图和数据块位图）
- ✅ 目录结构（支持多级目录）
- ✅ 文件创建、读写、删除
- ✅ 路径解析（支持绝对路径和相对路径）

### 用户管理

- ✅ 多用户支持
- ✅ 密码验证
- ✅ 权限控制（文件操作需要登录）
- ✅ 默认root用户

### 持久化存储

- ✅ 文件系统镜像保存/加载
- ✅ 自动保存到 `my_unix_fs.dat`
- ✅ 断电恢复功能

### 测试框架

- ✅ 位图操作测试
- ✅ 目录操作测试
- ✅ 文件操作测试
- ✅ 用户管理测试

## 使用示例

```bash
# 启动系统
./minifs.exe

# 登录
MiniFS:/> login root
请输入密码: root

# 查看帮助
MiniFS:root@/> help

# 创建目录
MiniFS:root@/> mkdir test_dir

# 切换目录
MiniFS:root@/> cd test_dir

# 创建文件
MiniFS:root@/test_dir> create hello.txt

# 打开文件写入
MiniFS:root@/test_dir> open hello.txt c
MiniFS:root@/test_dir> write 3 Hello World

# 读取文件
MiniFS:root@/test_dir> read 3 11

# 退出
MiniFS:root@/test_dir> exit
```

## 技术细节

### 文件系统布局

- 块0：超级块
- 块1：i-节点位图
- 块2：数据块位图
- 块3-18：i-节点区域（128个i-节点）
- 块19-1023：数据区域

### 存储参数

- 总块数：1024块
- 块大小：512字节
- i-节点大小：64字节
- 最大文件名长度：14字符
- 支持的最大文件大小：~6KB（12个直接块）

## 注意事项

1. **编译时务必选择正确的任务**：使用 "编译 minifs (C++)" 而非默认g++任务
2. **首次运行需要登录**：使用用户名 `root`，密码 `root`
3. **文件操作需要登录权限**：确保先执行 `login` 命令
4. **数据持久化**：程序退出时会自动保存到 `my_unix_fs.dat`
5. **路径格式**：支持绝对路径（/path/to/file）和相对路径（file）

## 故障排除

### 编译错误

- 确保选择了正确的编译任务："编译 minifs (C++)"
- 检查所有源文件是否存在

### 登录问题

- 确保用户名为 `root`（区分大小写）
- 确保密码为 `root`
- 如果忘记密码，可以使用 `format` 命令重置系统

### 权限错误

- 大部分文件操作需要先登录
- 使用 `whoami` 检查当前登录状态

---

**开发环境**：C++11或更高版本
**测试平台**：Windows + VS Code
**编译器**：g++
