// kernel/interrupts/interrupt.c
#include "interrupt.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "io.h"
#include "types.h"
#include "serial.h"


void keyboard_controller_init() {
    // 读取控制器状态
    while (inb(0x64) & 0x01) {
        inb(0x60);  // 清空输出缓冲区
    }

    // 发送 "Enable IRQ1" 命令
    outb(0x64, 0xAE);  // 0xAE = Enable Keyboard Interrupt

    // 可选：确认状态
    // outb(0x60, 0xF4);  // 发送 "Enable Scanning" 命令
}

// 填充中断向量桩（在 interrupt_asm.s 中生成）
void isr_install() {
    idt_install();

    pic_remap(0x20, 0x28);

    // 启用键盘（IRQ1）
    pic_clear_mask(1);

    keyboard_controller_init();
}
