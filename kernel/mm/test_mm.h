// kernel/mm/test_mm.h
#ifndef TEST_MM_H
#define TEST_MM_H

// 测试函数声明
void run_all_memory_tests(void);
void test_buddy_system(void);
void test_temp_mapping(void);
void test_kmalloc(void);
void test_integration(void);

#endif
