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
    fb_init();
    serial_init(COM1,1);          // 初始化串口                                                                                               
    serial_write(COM1, "MatrixOS Booting...\n");
    fb_puts("MatrixOS Booting...\n");

    // 初始化内存管理系统                                                                                                                     
    pmm_init(0x1000000, 0x2000000);  // 16MB - 32MB                                                                                          
    temp_mapping_init();
    kmalloc_init();

    serial_printf(COM1, "\nSystem ready for normal operation.\n");

    isr_install();              // 安装中断
    
    // 开启全局中断

    __asm__ volatile ("sti");   

    keyboard_init();

    serial_write(COM1, "Interrupts ENABLED!\n");
    fb_puts("Interrupts ENABLED!\n");

    while (1) {
        char c = keyboard_get_char();
        if (c){
            serial_printf(COM1, "Main loop got char: '%c' '%x'\n", c, c);
            // 可选：在屏幕上也回显
            //char buf[2] = {c, 0};
            fb_putchar(c);
        }
    }
}

