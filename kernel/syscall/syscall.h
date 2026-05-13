#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>

// System call numbers
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

/*
 * int 0x80: eax = syscall number; ebx/ecx/edx = args; return in eax.
 * See document/syscall-table.md for the full list.
 */

struct regs {
    uint32_t gs, fs, es, ds;      // segment registers
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // GPRs
    uint32_t int_no, err_code;    // vector and error code (if any)
    uint32_t eip, cs, eflags, useresp, ss;  // CPU-pushed on ring transition
};


// Syscall setup (called from assembly)
void syscall_init(void);

// Handler invoked from syscall_entry (assembly)
void syscall_handler(struct regs *r);

#endif