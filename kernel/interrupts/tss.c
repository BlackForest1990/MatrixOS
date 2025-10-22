#include "tss.h"
#include "serial.h"
#include "string.h"

// 正确定义 TSS 变量（去掉 static）
tss_entry_t tss;

void tss_init(void) {
    serial_printf(COM1, "TSS: Initializing with detailed setup...\n");
    
    memset(&tss, 0, sizeof(tss_entry_t));
    
    // 设置基本字段
    tss.ss0 = 0x10;  // 内核数据段
    tss.esp0 = 0xC0130000;  // 使用固定的安全内核栈地址
    
    // 设置其他段寄存器为内核段
    tss.es = 0x10;
    tss.cs = 0x08; 
    tss.ss = 0x10;
    tss.ds = 0x10;
    tss.fs = 0x10;
    tss.gs = 0x10;
    
    // 关键：I/O 权限位图
    tss.iomap_base = sizeof(tss_entry_t);
    
    serial_printf(COM1, "TSS: Complete setup:\n");
    serial_printf(COM1, "  ss0=0x%x, esp0=0x%x\n", tss.ss0, tss.esp0);
    serial_printf(COM1, "  cs=0x%x, ds=0x%x\n", tss.cs, tss.ds);
    serial_printf(COM1, "  iomap_base=0x%x\n", tss.iomap_base);
}

void tss_set_stack(uint32_t kernel_ss, uint32_t kernel_esp) {
    tss.ss0 = kernel_ss;
    tss.esp0 = kernel_esp;
    serial_printf(COM1, "TSS: Set kernel stack: SS=0x%x, ESP=0x%x\n", kernel_ss, kernel_esp);
}