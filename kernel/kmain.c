#include "types.h"
#include "drivers/fb.h"
#include "drivers/serial.h"

void kmain(void)
{
    // 初始化串口
    serial_init(COM1, 1);
    
    // 基本格式化输出
    serial_printf(COM1, "Hello, %s!\n", "World");
    serial_printf(COM1, "Number: %d, Hex: 0x%x\n", 42, 42);
    serial_printf(COM1, "Char: %c, Percent: %%\n", 'A');
    
    // 复杂格式化
    int a = 10, b = 20;
    serial_printf(COM1, "Sum: %d + %d = %d\n", a, b, a + b);
    
    // 分级日志
    serial_log(COM1, DEBUG, "Debug message: %s", "test");
    serial_log(COM1, INFO, "System started with %d MB memory", 1024);
    serial_log(COM1, ERROR, "Error at address: 0x%x", 0x7FFFFFFF);
    
    // 多参数示例
    serial_log(COM1, INFO, "Multiple params: %d, %s, 0x%x", 123, "test", 0xABCDEF);
}
