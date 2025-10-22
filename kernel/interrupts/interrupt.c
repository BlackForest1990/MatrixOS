// kernel/interrupts/interrupt.c
#include "interrupt.h"
#include "idt.h"
#include "pic.h"
#include "serial.h"
#include "tss.h" 

// 声明 TSS 加载函数
extern void tss_flush();

// 填充中断向量桩（在 interrupt_asm.s 中生成）
void isr_install() {

    idt_install();

    pic_remap(0x20, 0x28);

    //首先设置 TSS 描述符的基地址
    extern void tss_set_descriptor(uint32_t base);
    tss_set_descriptor((uint32_t)&tss);
    serial_printf(COM1, "TSS descriptor base set to 0x%x\n", (uint32_t)&tss);

    // 初始化 TSS
    tss_init();
    
    // 设置 TSS 描述符
    uint32_t tss_base = (uint32_t)&tss;
    
    // 更新 GDT 中的 TSS 描述符（需要汇编辅助）
    serial_printf(COM1, "INTERRUPT: Setting TSS at 0x%x\n", tss_base);
    
    // 加载 TSS
    tss_flush();
    
    // 设置内核栈到 TSS
    uint32_t kernel_esp;
    asm volatile("mov %%esp, %0" : "=r"(kernel_esp));
    tss_set_stack(0x10, kernel_esp);  // 0x10 = 内核数据段

    serial_printf(COM1, "TSS: Kernel stack set to ESP=0x%x\n", kernel_esp);


    // 启用键盘（IRQ1）
    pic_clear_mask(1);
}
