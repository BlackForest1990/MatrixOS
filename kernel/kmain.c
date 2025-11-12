#include "multiboot.h"
#include "fb.h"
#include "serial.h"
#include "interrupt.h"
#include "keyboard.h"
#include "pmm.h"
#include "temp_mapping.h"
#include "kmalloc.h"
#include "test_mm.h"
#include "process.h"
#include "loader.h"
#include "syscall.h"
#include "string.h"
#include "test_fs.h"
#include "vfs.h"
#include "ramfs.h"
#include "devfs.h"

void kmain(uint32_t magic, uint32_t addr) {
    fb_init();
    serial_init(COM1, 1);
    serial_write(COM1, "MatrixOS Booting...\n");
    fb_puts("MatrixOS Booting...\n");

    if (magic != 0x2BADB002) {
        serial_printf(COM1, "ERROR: Invalid multiboot magic: 0x%x\n", magic);
        return;
    }

    multiboot_info_t* mbi = (multiboot_info_t*)addr;
    
    pmm_init(0x1000000, 0x2000000);
    temp_mapping_init();
    kmalloc_init();
    process_init();
    syscall_init();

    // ✅ 修正：先加载GRUB模块，再初始化文件系统
    if (mbi->flags & (1 << 3)) {
        loader_init(mbi->mods_count, mbi->mods_addr);
        serial_printf(COM1, "Loaded %d user modules\n", loader_get_module_count());
    } else {
        serial_printf(COM1, "No user modules found in multiboot info\n");
    }

    // ✅ 修正：现在初始化文件系统（RAMFS需要访问已加载的模块）
    vfs_init();
    ramfs_init();
    devfs_init();

    // 运行文件系统测试
    run_filesystem_tests();

    isr_install();
    __asm__ volatile ("sti");
    keyboard_init();

    serial_printf(COM1, "\n=== System Ready ===\n");
    fb_puts("System Ready!\n");

    //run_all_memory_tests();

    // 在启动用户程序前，检查文件系统状态
    if (loader_get_module_count() > 0) {
        // 检查文件是否存在
        struct file_stat stat;
        if (vfs_stat("hello", &stat) == 0) {
            serial_printf(COM1, "Found user program 'hello' (%d bytes)\n", stat.size);
        } else {
            serial_printf(COM1, "User program 'hello' not found in filesystem\n");
        }
        
        uint32_t pid = process_create_from_module("hello");
        if (pid) {
            process_list();
            serial_printf(COM1, "\nStarting user program...\n");
            fb_puts("Starting user program...\n");
            process_start(pid);
        } else {
            serial_printf(COM1, "Failed to create process from module 'hello'\n");
        }
    } else {
        serial_printf(COM1, "No user modules found\n");
        fb_puts("No user modules found\n");
    }

    while (1) {
        char c = keyboard_get_char();
        if (c) {
            serial_printf(COM1, "Kernel: got '%c'\n", c);
            fb_putchar(c);
        }
        __asm__ volatile ("hlt");
    }
}