// kernel/kmain.c
#include "interrupt.h"
#include "serial.h"
#include "pic.h"
#include "keyboard.h" 

void kmain() {
    serial_init(COM1,1);          // 初始化串口
    serial_write(COM1, "MatrixOS Booting...\n");

    isr_install();              // 安装中断
    
    // 开启全局中断
    __asm__ volatile ("sti");

    keyboard_init();

    serial_write(COM1, "Interrupts ENABLED!\n");

    while (1);  // 等待中断
}
