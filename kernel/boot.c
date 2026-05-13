#include "boot.h"
#include "boot_config.h"
#include "multiboot.h"
#include "fb.h"
#include "serial.h"
#include "interrupt.h"
#include "keyboard.h"
#include "pmm.h"
#include "temp_mapping.h"
#include "kmalloc.h"
#include "process.h"
#include "loader.h"
#include "syscall.h"
#include "test_fs.h"
#include "vfs.h"
#include "ramfs.h"
#include "devfs.h"
#include "vfs.h"
#include "test_mm.h"

void boot_early_console(void) {
    fb_init();
    serial_init(COM1, 1);
    serial_write(COM1, "MatrixOS Booting...\n");
    fb_puts("MatrixOS Booting...\n");
}

bool boot_multiboot_ok(uint32_t magic) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        serial_printf(COM1, "ERROR: Invalid multiboot magic: 0x%x\n", magic);
        return false;
    }
    return true;
}

void boot_mm_init(void) {
    pmm_init(BOOT_PMM_PHYS_START, BOOT_PMM_PHYS_END);
    temp_mapping_init();
    kmalloc_init();
}

void boot_kernel_subsystems_init(void) {
    process_init();
    syscall_init();
}

void boot_modules_from_mbi(multiboot_info_t* mbi) {
    /* 先加载 GRUB 模块，再挂 RAMFS（模块需已由 loader 可见） */
    if (mbi->flags & BOOT_MBI_FLAG_MODULES) {
        loader_init(mbi->mods_count, mbi->mods_addr);
        serial_printf(COM1, "Loaded %d user modules\n", loader_get_module_count());
    } else {
        serial_printf(COM1, "No user modules found in multiboot info\n");
    }
}

void boot_vfs_init(void) {
    vfs_init();
    ramfs_init();
    devfs_init();
}

void boot_filesystem_demo(void) {
    run_filesystem_tests();
}

void boot_interrupts_and_input(void) {
    isr_install();
    __asm__ volatile("sti");
    keyboard_init();
}

void boot_print_ready(void) {
    serial_printf(COM1, "\n=== System Ready ===\n");
    fb_puts("System Ready!\n");
}

void boot_optional_memory_tests(void) {
#if BOOT_DEMO_RUN_MEMORY_TESTS
    run_all_memory_tests();
#endif
}

void boot_user_demo(void) {
    if (loader_get_module_count() == 0) {
        serial_printf(COM1, "No user modules found\n");
        fb_puts("No user modules found\n");
        return;
    }

    struct file_stat stat;
    if (vfs_stat(BOOT_DEMO_USER_MODULE_NAME, &stat) == 0) {
        serial_printf(COM1, "Found user program '%s' (%d bytes)\n",
                      BOOT_DEMO_USER_MODULE_NAME, stat.size);
    } else {
        serial_printf(COM1, "User program '%s' not found in filesystem\n",
                      BOOT_DEMO_USER_MODULE_NAME);
    }

    uint32_t pid = process_create_from_module(BOOT_DEMO_USER_MODULE_NAME);
    if (pid) {
        process_list();
        serial_printf(COM1, "\nStarting user program...\n");
        fb_puts("Starting user program...\n");
        process_start(pid);
    } else {
        serial_printf(COM1, "Failed to create process from module '%s'\n",
                      BOOT_DEMO_USER_MODULE_NAME);
    }
}

void boot_idle_forever(void) {
    while (1) {
        char c = keyboard_get_char();
        if (c) {
            serial_printf(COM1, "Kernel: got '%c'\n", c);
            fb_putchar(c);
        }
        __asm__ volatile("hlt");
    }
}
