#include "multiboot.h"
#include "boot.h"
#include "boot_config.h"

void kmain(uint32_t magic, uint32_t addr) {
    multiboot_info_t* mbi = (multiboot_info_t*)addr;

    boot_early_console();
    if (!boot_multiboot_ok(magic)) {
        return;
    }

    boot_mm_init();
    boot_kernel_subsystems_init();
    boot_modules_from_mbi(mbi);
    boot_vfs_init();

#if BOOT_DEMO_RUN_FILESYSTEM_TESTS
    boot_filesystem_demo();
#endif

    boot_interrupts_and_input();
    boot_print_ready();
    boot_optional_memory_tests();

#if BOOT_DEMO_RUN_USER_PROGRAM
    boot_user_demo();
#endif

    boot_idle_forever();
}
