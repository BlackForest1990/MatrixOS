// kernel/interrupts/pic.c
#include "pic.h"
#include "io.h"
#include "types.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x11
#define ICW4_8086 0x01

// pic.c
void pic_remap(int offset1, int offset2) {
    uint8_t a1, a2;

    a1 = inb(0x21);  // 保存 IMR
    a2 = inb(0xA1);

    // ICW1: 边沿触发，级联，需要 ICW4
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2: 设置中断向量偏移
    outb(0x21, offset1);  // 主片：0x20-0x27
    outb(0xA1, offset2);  // 从片：0x28-0x2F

    // ICW3: 设置级联
    outb(0x21, 0x04);  // 主片 IR2 连接到从片
    outb(0xA1, 0x02);  // 从片连接到主片 IR2

    // ICW4: 8086 模式`
    outb(0x21, 0x01);  // 主片
    outb(0xA1, 0x01);  // 从片
    
    // 屏蔽所有中断（除了后面要启用的）
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);     
  
    // 恢复 IMR
    outb(0x21, a1);
    outb(0xA1, a2);
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) {                 // ✅ 正确：IRQ 编号 >= 8 表示来自 PIC2
        outb(PIC2_CMD, 0x20);       // 发送 EOI 给 PIC2
    }
    outb(PIC1_CMD, 0x20);           // 所有中断都必须给 PIC1 发 EOI
}

void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
