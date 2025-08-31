#ifndef SERIAL_H
#define SERIAL_H

#include <types.h>

/* 串口端口地址 */
#define COM1 0x3F8

/* 日志级别 */
typedef enum {
    DEBUG,
    INFO, 
    ERROR
} log_level_t;

/* 可变参数相关类型 */
typedef char* va_list;
#define va_start(ap, param) (ap = (va_list)&param + sizeof(param))
#define va_arg(ap, type) (*(type*)((ap += sizeof(type)) - sizeof(type)))
#define va_end(ap) (ap = (va_list)0)

/* 初始化串口 */
void serial_init(uint16_t port, uint16_t divisor);

/* 检查传输FIFO是否为空 */
int serial_transmit_empty(uint16_t port);

/* 向串口写入单个字符 */
void serial_write_char(uint16_t port, char c);

/* 向串口写入字符串 */
void serial_write(uint16_t port, const char* str);

/* 类似printf的串口输出函数 */
void serial_printf(uint16_t port, const char* format, ...);
void serial_vprintf(uint16_t port, const char* format, va_list args);

/* 带日志级别的串口输出 */
void serial_log(uint16_t port, log_level_t level, const char* format, ...);

#endif /* SERIAL_H */
