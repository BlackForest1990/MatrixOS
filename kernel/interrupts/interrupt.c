// kernel/interrupts/interrupt.c
#include "interrupt.h"
#include "idt.h"
#include "pic.h"

// 填充中断向量桩（在 interrupt_asm.s 中生成）
void isr_install() {

    idt_install();

    pic_remap(0x20, 0x28);

    // 启用键盘（IRQ1）
    pic_clear_mask(1);
}
