#ifndef FS_TESTS_HPP
#define FS_TESTS_HPP

class MiniFS; // 前向声明 MiniFS 类

void test_bitmap_operations(MiniFS& fs);
void test_directory_operations(MiniFS& fs);
void test_file_operations(MiniFS& fs);
// 测试MiniFS中的用户管理功能
void test_user_operations(MiniFS& fs);

#endif // FS_TESTS_HPP