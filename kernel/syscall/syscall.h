#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>

// 系统调用号定义
#define SYSCALL_EXIT     1
#define SYSCALL_PUTS     2
#define SYSCALL_GETC     3
#define SYSCALL_PUTCHAR  4
#define SYSCALL_GETPID   5

#define SYSCALL_OPEN    10
#define SYSCALL_CLOSE   11
#define SYSCALL_READ    12
#define SYSCALL_WRITE   13
#define SYSCALL_SEEK    14
#define SYSCALL_IOCTL   15
#define SYSCALL_STAT    16

struct regs {
    uint32_t gs, fs, es, ds;      // 段寄存器
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // 通用寄存器
    uint32_t int_no, err_code;    // 中断号和错误码
    uint32_t eip, cs, eflags, useresp, ss;  // 自动压栈的寄存器
};


// 系统调用初始化
void syscall_init(void);

// 系统调用处理函数（由汇编调用）
void syscall_handler(struct regs *r);

#endif