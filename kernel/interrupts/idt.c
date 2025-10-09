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
    // 设置所有256个IDT条目
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)interrupt_vectors[i], 0x8E);
    }

    // 加载 IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}
