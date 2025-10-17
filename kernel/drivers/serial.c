#include "serial.h"
#include "types.h"
#include "io.h"
#include "string.h"

/* 串口初始化 */
void serial_init(uint16_t port, uint16_t divisor) {
    outb(port + 1, 0x00);        // 禁用中断
    outb(port + 3, 0x80);        // 启用DLAB
    outb(port + 0, divisor & 0xFF);     // 除数低位
    outb(port + 1, (divisor >> 8) & 0xFF); // 除数高位
    outb(port + 3, 0x03);        // 8位数据，无奇偶校验，1停止位
    outb(port + 2, 0xC7);        // 启用FIFO
    outb(port + 4, 0x03);        // 启用RTS和DTR
}

/* 检查传输FIFO是否为空 */
int serial_transmit_empty(uint16_t port) {
    return inb(port + 5) & 0x20;
}

/* 向串口写入单个字符 */
void serial_write_char(uint16_t port, char c) {
    while (!serial_transmit_empty(port));
    outb(port, c);
}

/* 向串口写入字符串 */
void serial_write(uint16_t port, const char* str) {
    while (*str) {
        serial_write_char(port, *str++);
    }
}

/* 整数转字符串（十进制） */
static void itoa_dec(int num, char* str) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int is_negative = 0;
    
    if (num == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return;
    }
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    do {
        *ptr++ = '0' + (num % 10);
        num /= 10;
    } while (num > 0);
    
    if (is_negative) {
        *ptr++ = '-';
    }
    
    *ptr-- = '\0';
    
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

/* 整数转字符串（十六进制） */
static void itoa_hex(uint32_t num, char* str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    char* ptr = str;
    int digit;
    int count = 0;

    do {
        digit = num & 0xF;
        *ptr++ = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        num >>= 4;
        count++;
    } while (num && count < 8);

    *ptr-- = '\0';

    // 反转
    char* start = str;
    while (start < ptr) {
        char tmp = *start;
        *start++ = *ptr;
        *ptr-- = tmp;
    }
}

/* 核心格式化输出函数 */
void serial_vprintf(uint16_t port, const char* format, va_list args) {
    for (const char* p = format; *p; p++) {
        if (*p != '%') {
            serial_write_char(port, *p);
            continue;
        }
        
        p++; // 跳过'%'
        switch (*p) {
            case 'd': {
                int num = va_arg(args, int);
                char buffer[16];
                itoa_dec(num, buffer);
                serial_write(port, buffer);
                break;
            }
            case 'x': {
               uint32_t num = va_arg(args, uint32_t);
               char buffer[16];
               itoa_hex(num, buffer);
               serial_write(port, buffer);
               break;
            }
            case 's': {
                char* str = va_arg(args, char*);
                serial_write(port, str);
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                serial_write_char(port, c);
                break;
            }
            case 'p': { 
                uint32_t ptr = va_arg(args, uint32_t);
                char buffer[16];
                buffer[0] = '0';
                buffer[1] = 'x';
                itoa_hex(ptr, buffer + 2);  // 假设 itoa_hex 不带 0x
                serial_write(port, buffer);
                break;
            }
            case '%': {
                serial_write_char(port, '%');
                break;
            }
            default:
                serial_write_char(port, '%');
                serial_write_char(port, *p);
                break;
        }
    }
}

/* 可变参数printf函数 */
void serial_printf(uint16_t port, const char* format, ...) {
    va_list args;
    va_start(args, format);
    serial_vprintf(port, format, args);
    va_end(args);
}

/* 带日志级别的可变参数输出 */
void serial_log(uint16_t port, log_level_t level, const char* format, ...) {
    switch (level) {
        case DEBUG:
            serial_write(port, "[DEBUG] ");
            break;
        case INFO:
            serial_write(port, "[INFO] ");
            break;
        case ERROR:
            serial_write(port, "[ERROR] ");
            break;
    }
    
    va_list args;
    va_start(args, format);
    serial_vprintf(port, format, args);
    va_end(args);
    
    serial_write(port, "\n");
}
