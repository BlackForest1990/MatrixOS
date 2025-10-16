#include "interrupt.h"
#include "serial.h"
#include "pic.h"
#include "keyboard.h"
#include "fb.h"
#include "pmm.h"
#include "temp_mapping.h"
#include "kmalloc.h"
#include "string.h"
#include "test_mm.h"

void kmain() {

    serial_init(COM1,1);          // 初始化串口                                                                                               
    serial_write(COM1, "MatrixOS Booting...\n");

    // 初始化内存管理系统                                                                                                                     
    pmm_init(0x1000000, 0x2000000);  // 16MB - 32MB                                                                                          
    temp_mapping_init();
    kmalloc_init();

    // 运行所有测试                                                                                                                           
    run_all_memory_tests();

    serial_printf(COM1, "\nSystem ready for normal operation.\n");

    while(1) {
        asm volatile("hlt");
    }
}

