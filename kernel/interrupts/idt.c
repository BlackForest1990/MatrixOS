// kernel/interrupts/idt.c
#include "idt.h"
#include "io.h"
#include "serial.h"

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idt_ptr;

extern void* interrupt_vectors[];
// 声明外部函数
extern void syscall_entry();  // 系统调用汇编入口

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

    serial_printf(COM1, "IDT: Starting installation...\n");
    serial_printf(COM1, "  IDT location: 0x%x\n", (uint32_t)&idt);
    serial_printf(COM1, "  IDT pointer: base=0x%x, limit=0x%x\n", idt_ptr.base, idt_ptr.limit);
    
    // 检查关键符号地址
    serial_printf(COM1, "  interrupt_vectors[0]: 0x%x\n", (uint32_t)interrupt_vectors[0]);
    serial_printf(COM1, "  syscall_entry: 0x%x\n", (uint32_t)syscall_entry);
    serial_printf(COM1, "  interrupt_vectors[0x80]: 0x%x\n", (uint32_t)interrupt_vectors[0x80]);

    // 🔍 调试：打印 interrupt_vectors[33] 地址
    //serial_printf(COM1, "int_33 vector addr: %p\n", interrupt_vectors[0x21]);
    // 设置所有256个IDT条目
    for (int i = 0; i < 256; i++) {
        if(i == 0x80){
            // 安装系统调用中断 (0x80)
            // 使用 0xEE 作为 flags: P=1, DPL=3, Type=0xE (32位中断门)
            // 这样用户模式 (DPL=3) 才能调用这个中断
            serial_printf(COM1, "  Setting INT 0x80 to syscall_entry (0x%x)\n", (uint32_t)syscall_entry);
            idt_set_gate(0x80, (uint32_t)syscall_entry, 0xEE);
            serial_printf(COM1, "INTERRUPT: System call handler installed at 0x80\n");

        }else{
            idt_set_gate(i, (uint32_t)interrupt_vectors[i], 0x8E);
        }
    }
    // 验证设置
    serial_printf(COM1, "  IDT[0x80]: base_low=0x%x, base_high=0x%x, flags=0x%x\n",
                 idt[0x80].base_low, idt[0x80].base_high, idt[0x80].flags);

    // 加载 IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));

    serial_printf(COM1, "IDT: Installation complete\n");
}
