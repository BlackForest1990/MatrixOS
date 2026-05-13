#ifndef BOOT_H
#define BOOT_H

#include "multiboot.h"
#include "types.h"

void boot_early_console(void);
bool boot_multiboot_ok(uint32_t magic);
void boot_mm_init(void);
void boot_kernel_subsystems_init(void);
void boot_modules_from_mbi(multiboot_info_t* mbi);
void boot_vfs_init(void);
void boot_filesystem_demo(void);
void boot_interrupts_and_input(void);
void boot_print_ready(void);
void boot_optional_memory_tests(void);
void boot_user_demo(void);
void boot_idle_forever(void);

#endif
