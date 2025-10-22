#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>

// 系统调用号定义
#define SYSCALL_EXIT     1
#define SYSCALL_PUTS     2
#define SYSCALL_GETC     3
#define SYSCALL_PUTCHAR  4
#define SYSCALL_GETPID   5

// 系统调用初始化
void syscall_init(void);

// 系统调用处理函数（由汇编调用）
void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#endif