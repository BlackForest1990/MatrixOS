#include "syscall.h"
#include "process.h"
#include "serial.h"
#include "fb.h"
#include "keyboard.h"

void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    (void)arg2;
    (void)arg3;
    pcb_t* current = process_get_current();
    uint32_t pid = current ? current->pid : 0;
    
    serial_printf(COM1, "SYSCALL: Process %d called syscall %d\n", pid, syscall_num);
    
    switch (syscall_num) {
        case SYSCALL_EXIT:
            serial_printf(COM1, "SYSCALL: Process %d exiting\n", pid);
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
                if (c == '\n') {
                    serial_printf(COM1, "[USER%d OUTPUT] newline\n", pid);
                    fb_putchar('\n');
                } else {
                    serial_printf(COM1, "[USER%d OUTPUT] '%c'\n", pid, c);
                    fb_putchar(c);
                }
            }
            break;
            
        case SYSCALL_GETC:
            {
                // 简单的键盘输入（非阻塞）
                char c = keyboard_get_char();
                if (c) {
                    serial_printf(COM1, "SYSCALL: Process %d read char '%c'\n", pid, c);
                    // 这里应该通过某种机制返回给用户程序
                }
            }
            break;
            
        case SYSCALL_GETPID:
            serial_printf(COM1, "SYSCALL: Process %d requested PID\n", pid);
            // 返回值可以通过修改寄存器在汇编中处理
            break;
            
        default:
            serial_printf(COM1, "SYSCALL: Unknown system call %d from process %d\n", 
                         syscall_num, pid);
            break;
    }
}

void syscall_init(void) {
    serial_printf(COM1, "SYSCALL: System call interface initialized\n");
    serial_printf(COM1, "INT 0x80 registered with DPL=3 for user mode access\n");
}