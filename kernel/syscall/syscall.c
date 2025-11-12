#include "syscall.h"
#include "process.h"
#include "serial.h"
#include "fb.h"
#include "keyboard.h"
#include "vfs.h"

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

            serial_printf(COM1, "SYSCALL_PUTS: direct access to user pointer 0x%x\n", arg1);
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

        case SYSCALL_OPEN:
            if (arg1) {
                char* filename = (char*)arg1;
                int fd = vfs_open(filename, arg2);
                r->eax = fd;
                serial_printf(COM1, "SYSCALL: Process %d opened '%s' (flags: 0x%x) -> fd %d\n", 
                            pid, filename, arg2, fd);
            } else {
                r->eax = EINVAL;
            }
            break;

        case SYSCALL_CLOSE:
            r->eax = vfs_close(arg1);
            serial_printf(COM1, "SYSCALL: Process %d closed fd %d\n", pid, arg1);
            break;

        case SYSCALL_READ:
            {
                int fd = arg1;
                char* buf = (char*)arg2;
                uint32_t count = arg3;
                
                if (!buf) {
                    r->eax = EINVAL;
                    break;
                }
                
                int bytes_read = vfs_read(fd, buf, count);
                r->eax = bytes_read;
                
                if (bytes_read > 0) {
                    serial_printf(COM1, "SYSCALL: Process %d read %d bytes from fd %d\n", 
                                pid, bytes_read, fd);
                }
            }
            break;
        case SYSCALL_WRITE:
            {
                int fd = arg1;
                const char* buf = (const char*)arg2;
                uint32_t count = arg3;
                
                if (!buf) {
                    r->eax = EINVAL;
                    break;
                }
                
                int bytes_written = vfs_write(fd, buf, count);
                r->eax = bytes_written;
                
                if (bytes_written > 0 && fd != STDOUT_FILENO && fd != STDERR_FILENO) {
                    serial_printf(COM1, "SYSCALL: Process %d wrote %d bytes to fd %d\n", 
                                pid, bytes_written, fd);
                }
            }
            break;

        case SYSCALL_SEEK:
            {
                int fd = arg1;
                int offset = arg2;
                int whence = arg3;
                r->eax = vfs_seek(fd, offset, whence);
            }
            break;

        case SYSCALL_IOCTL:
            {
                int fd = arg1;
                int request = arg2;
                void* argp = (void*)arg3;
                r->eax = vfs_ioctl(fd, request, argp);
            }
            break;

        case SYSCALL_STAT:
            {
                const char* path = (const char*)arg1;
                struct file_stat* stat = (struct file_stat*)arg2;
                
                if (!path || !stat) {
                    r->eax = EINVAL;
                    break;
                }
                
                r->eax = vfs_stat(path, stat);
            }
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