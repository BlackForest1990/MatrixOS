// kernel/interrupts/keyboard.c
#include "keyboard.h"
#include "io.h"
#include "serial.h"  // 假设你有串口输出函数
#include "types.h"
#include "pic.h" 

#define KEYBOARD_PORT 0x60

// 环形缓冲区大小
#define KBUF_SIZE 256

// 键盘输入缓冲区（存储 ASCII 字符）
static char kbuf[KBUF_SIZE] = {0};
static int khead = 0;
static int ktail = 0;

// 普通模式下的字符映射表（扫描码 -> ASCII）
const char normal_map[128] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b',
    '\t', 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\n',
    0,    'a',  's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  '\'', '`',  0,
    '\\', 'z',  'x',  'c',  'v',  'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  0,    ' ',
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

/**
 * 将扫描码转换为 ASCII 字符
 * 处理 Shift、Caps Lock、Backspace、Enter 等
 *
 * @param sc 扫描码（0x00 ~ 0xFF）
 * @return 对应的 ASCII 字符，无字符时返回 0
 */
char scancode_to_char(uint8_t sc) {
    serial_printf(COM1, "scancode_to_char: '%d'\n", sc);
    // 释放事件：更新修饰键状态，不产生字符
    if (sc & 0x80) {
        
        return 0;
    }

    // 处理按下事件
    switch (sc) {
        case 0x0E:  // Backspace
            return '\b';

        case 0x1C:  // Enter
            return '\n';

        default:
            if (sc >= 128) return 0;
            break;
    }

    char c = normal_map[sc];
    if (!c) return 0;  // 无效键

    return c;
}

void keyboard_input_handler(uint8_t sc)  {
    // 🔥 过滤键盘响应，不是按键
    if (sc == 0xFA || sc == 0xAA || sc == 0x00 || sc == 0xFF) {
        // 是键盘的 ACK 或状态响应，不是按键
        pic_eoi(1);
        return;  // 直接返回，不写入 kbuf
    }
    char c = scancode_to_char(sc);
    if (c != 0) {
        serial_printf(COM1, "char c: '%c'\n", c);
        // 存入环形缓冲区
        kbuf[khead] = c;
        khead = (khead + 1) % KBUF_SIZE;
    }
    // 发送 EOI 给 PIC
    pic_eoi(1);
}

/**
 * 从键盘缓冲区获取一个字符（非阻塞）
 *
 * @return 字符，无数据时返回 0
 */
char keyboard_get_char(void) {
    serial_write(COM1, "keyboard_get_char\n");
    if (khead == ktail) return 0;
    char c = kbuf[ktail];
    ktail = (ktail + 1) % KBUF_SIZE;
    return c;
}

void interrupt_handler(void* regs, uint32_t int_no) {
    (void)regs;
   // serial_printf(COM1, "ISR: %x\n", int_no);

    if (int_no == 33) {
        serial_printf(COM1, "ISR: %d\n", int_no);
        uint8_t sc = inb(KEYBOARD_PORT);
        keyboard_input_handler(sc);  // 处理并自动 EOI
    }
}

void keyboard_init() {
    int timeout;

    //serial_write(COM1, "KB: Init start\n");

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

    serial_write(COM1, "KB: Init done.\n");
}
