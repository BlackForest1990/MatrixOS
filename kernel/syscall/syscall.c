#include "syscall.h"
#include "process.h"
#include "serial.h"
#include "fb.h"
#include "keyboard.h"

void syscall_handler(struct regs *r) {
    uint32_t syscall_num = r->eax;
    uint32_t arg1 = r->ebx;
    uint32_t arg2 = r->ecx;
    uint32_t arg3 = r->edx;
    
    pcb_t* current = process_get_current();
    uint32_t pid = current ? current->pid : 0;
    
    serial_printf(COM1, "SYSCALL: Process %d called syscall %d (args: 0x%x, 0x%x, 0x%x)\n", 
                  pid, syscall_num, arg1, arg2, arg3);
    
    switch (syscall_num) {
        case SYSCALL_EXIT:
            serial_printf(COM1, "SYSCALL: Process %d exiting with code %d\n", pid, arg1);
            process_exit_current();
            break;
            
        case SYSCALL_PUTS:
            if (arg1) {
                char* str = (char*)arg1;
                serial_printf(COM1, "[USER%d OUTPUT] %s\n", pid, str);
                fb_puts("[USER] ");
                fb_puts(str);
                fb_puts("\n");
            }
            break;
            
        case SYSCALL_PUTCHAR:
            {
                char c = (char)arg1;
                serial_printf(COM1, "[USER%d OUTPUT] char '%c' (0x%x)\n", pid, c, arg1);
                fb_putchar(c);
            }
            break;
            
        case SYSCALL_GETC:
            {
                char c = keyboard_get_char();
                if (c) {
                    serial_printf(COM1, "SYSCALL: Process %d read char '%c'\n", pid, c);
                    r->eax = (uint32_t)c;  // 通过 eax 返回值
                } else {
                    r->eax = 0;  // 没有输入
                }
            }
            break;
            
        case SYSCALL_GETPID:
            serial_printf(COM1, "SYSCALL: Process %d requested PID\n", pid);
            r->eax = pid;  // 通过 eax 返回 PID
            break;
            
        default:
            serial_printf(COM1, "SYSCALL: Unknown system call %d from process %d\n", 
                         syscall_num, pid);
            r->eax = (uint32_t)-1;  // 返回错误
            break;
    }
}

void syscall_init(void) {
    serial_printf(COM1, "SYSCALL: System call interface initialized\n");
    serial_printf(COM1, "INT 0x80 registered with DPL=3 for user mode access\n");
}