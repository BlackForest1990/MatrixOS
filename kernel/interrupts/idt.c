// kernel/interrupts/idt.c
#include "idt.h"
#include "io.h"
#include "serial.h"

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idt_ptr;

extern void* interrupt_vectors[];

void idt_set_gate(int n, uint32_t handler, uint8_t flags) {
    idt[n].base_low  = handler & 0xFFFF;
    idt[n].base_high = (handler >> 16) & 0xFFFF;
    idt[n].sel       = KERNEL_CS;
    idt[n].zero      = 0;
    idt[n].flags     = flags;
}

void idt_install() {
    idt_ptr.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idt_ptr.base  = (uint32_t)&idt;

    // 🔍 调试：打印 interrupt_vectors[33] 地址
    //serial_printf(COM1, "int_33 vector addr: %p\n", interrupt_vectors[0x21]);

    // 启用键盘中断（IRQ1 -> IDT[0x21]）
    idt_set_gate(0x20, (uint32_t)interrupt_vectors[0x20], 0x8E);
    idt_set_gate(0x21, (uint32_t)interrupt_vectors[0x21], 0x8E);  // Present, DPL=0, 32-bit Gate

    // 🔍 调试：打印 IDT[33] 内容
    //struct idt_entry* entry = &idt[0x21];  // IDT[33]
    //serial_printf(COM1, "IDT[33] base_low:  %x\n", entry->base_low);
    //serial_printf(COM1, "IDT[33] base_high: %x\n", entry->base_high);
    //serial_printf(COM1, "IDT[33] sel:       %x\n", entry->sel);
    //serial_printf(COM1, "IDT[33] flags:     %x\n", entry->flags);

    // 加载 IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}
