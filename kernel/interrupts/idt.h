// kernel/interrupts/idt.h
#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_ENTRIES 256
#define KERNEL_CS 0x08

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_set_gate(int n, uint32_t handler, uint8_t flags);
void idt_install();

#endif
