#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H

#include "types.h"

/*
 * 集中放置「第一节课就会改」的引导与演示常量。
 * 可在 Makefile 里用 CONFIG_DEMO_STARTUP=0 关闭演示，得到最短启动路径。
 */
#ifndef CONFIG_DEMO_STARTUP
#define CONFIG_DEMO_STARTUP 1
#endif

#if CONFIG_DEMO_STARTUP
#define BOOT_DEMO_RUN_FILESYSTEM_TESTS 1
#define BOOT_DEMO_RUN_USER_PROGRAM 1
#else
#define BOOT_DEMO_RUN_FILESYSTEM_TESTS 0
#define BOOT_DEMO_RUN_USER_PROGRAM 0
#endif

/* 默认关闭；改为 1 则在 System Ready 之后跑完整内存自测（串口输出较多） */
#ifndef BOOT_DEMO_RUN_MEMORY_TESTS
#define BOOT_DEMO_RUN_MEMORY_TESTS 0
#endif

/* 伙伴系统管理的物理内存区间 */
#define BOOT_PMM_PHYS_START 0x01000000u
#define BOOT_PMM_PHYS_END 0x02000000u

/* Multiboot info：模块列表有效位 */
#define BOOT_MBI_FLAG_MODULES (1u << 3)

/* RAMFS 中用户演示程序名（与 GRUB module / Makefile iso 目标一致） */
#define BOOT_DEMO_USER_MODULE_NAME "hello"

/* 第二个模块默认名（loader 在 GRUB 未提供模块名时的回退，与 iso 中 file_test 一致） */
#define BOOT_DEMO_USER_MODULE_NAME_2 "file_test"

#endif
