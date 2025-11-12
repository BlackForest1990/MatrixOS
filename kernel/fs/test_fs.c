// kernel/fs/test_fs.c
#include "test_fs.h"
#include "vfs.h"
#include "ramfs.h"
#include "devfs.h"
#include "serial.h"
#include "string.h"

void test_filesystem_integration(void) {
    serial_printf(COM1, "\n=== Filesystem Integration Test ===\n");
    
    // 测试1: 文件状态查询
    serial_printf(COM1, "Test 1: File stat...\n");
    struct file_stat stat;
    int result = vfs_stat("hello", &stat);
    if (result == 0) {
        serial_printf(COM1, "  File 'hello': size=%d, mode=0x%x, type=%d\n", 
                     stat.size, stat.mode, stat.type);
    } else {
        serial_printf(COM1, "  ERROR: Cannot stat file 'hello'\n");
    }
    
    // 测试2: 打开和读取文件
    serial_printf(COM1, "Test 2: File open and read...\n");
    int fd = vfs_open("hello", O_RDONLY);
    if (fd >= 0) {
        char buffer[128];
        int bytes_read = vfs_read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            serial_printf(COM1, "  Read %d bytes: %s\n", bytes_read, buffer);
        }
        vfs_close(fd);
    } else {
        serial_printf(COM1, "  ERROR: Cannot open file 'hello'\n");
    }
    
    // 测试3: 设备文件操作
    serial_printf(COM1, "Test 3: Device files...\n");
    int console_fd = vfs_open("/dev/console", O_WRONLY);
    if (console_fd >= 0) {
        const char* msg = "Kernel: Writing to console via VFS!\n";
        vfs_write(console_fd, msg, strlen(msg));
        vfs_close(console_fd);
        serial_printf(COM1, "  Console device test passed\n");
    } else {
        serial_printf(COM1, "  ERROR: Cannot open /dev/console\n");
    }
    
    // 测试4: 标准输出
    serial_printf(COM1, "Test 4: Standard output...\n");
    const char* stdout_msg = "Kernel: This goes to stdout\n";
    vfs_write(STDOUT_FILENO, stdout_msg, strlen(stdout_msg));
    
    serial_printf(COM1, "=== Filesystem Integration Test Complete ===\n");
}

void run_filesystem_tests(void) {
    serial_printf(COM1, "\n");
    serial_printf(COM1, "=========================================\n");
    serial_printf(COM1, "Starting Filesystem Tests\n");
    serial_printf(COM1, "=========================================\n");
    
    test_filesystem_integration();
    
    serial_printf(COM1, "\n");
    serial_printf(COM1, "=========================================\n");
    serial_printf(COM1, "Filesystem Tests Completed!\n");
    serial_printf(COM1, "=========================================\n");
}