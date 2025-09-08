// kernel/interrupts/keyboard.c
#include "keyboard.h"
#include "io.h"
#include "serial.h"  // 假设你有串口输出函数
#include "types.h"
#include "pic.h" 

#define KEYBOARD_PORT 0x60

// 简单的扫描码到 ASCII 映射（仅支持主键盘区，未处理 Shift）
static char scancode_to_ascii[] = {
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handler() {
    uint8_t scancode = inb(KEYBOARD_PORT);

    if (scancode & 0x80) {
        // 键释放，忽略
        return;
    } else {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            serial_write_char(COM1, c);  // 输出到串口
        }
    }

    // 发送 EOI 给 PIC
    pic_eoi(1);
}

void interrupt_handler(void* regs, uint32_t int_no) {
    (void)regs;
    //serial_printf(COM1, "ISR: %d\n", int_no);

    if (int_no == 33) {
        serial_write(COM1, "KEY: ");
        uint8_t sc = inb(0x60);
        char hex[3] = {0};
        hex[0] = "0123456789ABCDEF"[sc >> 4];
        hex[1] = "0123456789ABCDEF"[sc & 0xF];
        serial_write(COM1, hex);
        serial_write(COM1, "\n");
        keyboard_handler();
    }
}

void keyboard_init() {
    int timeout;

    serial_write(COM1, "KB: Init start\n");

    // 等待输入缓冲区空（发送命令前）
    timeout = 10000;
    while (--timeout && (inb(0x64) & 0x02));
    if (timeout <= 0) {
        serial_write(COM1, "KB: Timeout1\n");
        return;
    }

    serial_write(COM1, "KB: Disabling KB\n");
    outb(0x64, 0xAD);  // Disable Keyboard

    // 等待缓冲区空
    timeout = 10000;
    while (--timeout && (inb(0x64) & 0x02));
    if (timeout <= 0) {
        serial_write(COM1, "KB: Timeout2\n");
        return;
    }

    outb(0x64, 0xA7);  // Disable Aux

    // 清空输出缓冲区
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    serial_write(COM1, "KB: Enabling KB\n");
    outb(0x64, 0xAE);  // Enable Keyboard

    timeout = 10000;
    while (--timeout && (inb(0x64) & 0x02));
    if (timeout <= 0) {
        serial_write(COM1, "KB: Timeout3\n");
        return;
    }

    serial_write(COM1, "KB: Sending F4\n");
    outb(0x60, 0xF4);  // Enable Scanning

    // 等待 ACK
    timeout = 10000;
    while (--timeout && !(inb(0x64) & 0x01));
    if (timeout <= 0) {
        serial_write(COM1, "KB: No data from keyboard\n");
        return;
    }

    uint8_t ack = inb(0x60);
    char hex[3] = {0};
    hex[0] = "0123456789ABCDEF"[ack >> 4];
    hex[1] = "0123456789ABCDEF"[ack & 0xF];
    serial_write(COM1, "KB: ACK = ");
    serial_write(COM1, hex);
    serial_write(COM1, "\n");

    serial_write(COM1, "KB: Init done.\n");
}
