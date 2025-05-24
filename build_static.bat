@echo off
chcp 65001 >nul
echo ========== 静态编译 MiniFS ==========

REM 检查常见的编译器路径
if exist "C:\MinGW\bin\g++.exe" (
    set COMPILER_PATH=C:\MinGW\bin\g++.exe
    goto compile
)

if exist "C:\mingw64\bin\g++.exe" (
    set COMPILER_PATH=C:\mingw64\bin\g++.exe
    goto compile
)

if exist "C:\msys64\mingw64\bin\g++.exe" (
    set COMPILER_PATH=C:\msys64\mingw64\bin\g++.exe
    goto compile
)

REM 尝试使用PATH中的g++
where g++ >nul 2>&1
if %errorlevel% == 0 (
    set COMPILER_PATH=g++
    goto compile
)

echo 错误: 未找到 g++ 编译器
echo 请确保已安装 MinGW 或其他 C++ 编译器
pause
exit /b 1

:compile
echo 使用编译器: %COMPILER_PATH%
echo.

REM 编译命令
echo 正在编译...
%COMPILER_PATH% -std=c++11 -O2 -static -static-libgcc -static-libstdc++ ^
    main.cpp minifs.cpp fs_tests.cpp shell_utils.cpp user.cpp ^
    -o minifs.exe

if %errorlevel% == 0 (
    echo.
    echo ========== 编译成功! ==========
    echo 可执行文件: minifs.exe
    echo.
    echo 文件大小:
    dir minifs.exe | findstr minifs.exe
    echo.
    echo 测试运行:
    echo minifs.exe --test
    echo.
) else (
    echo.
    echo ========== 编译失败! ==========
    echo 请检查源代码是否有错误
)

pause
