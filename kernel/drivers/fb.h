#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "types.h"  /* 使用自定义类型 */

/* 颜色常量定义 */
#define FB_BLACK         0x0
#define FB_BLUE          0x1
#define FB_GREEN         0x2
#define FB_CYAN          0x3
#define FB_RED           0x4
#define FB_MAGENTA       0x5
#define FB_BROWN         0x6
#define FB_LIGHT_GRAY    0x7
#define FB_DARK_GRAY     0x8
#define FB_LIGHT_BLUE    0x9
#define FB_LIGHT_GREEN   0xA
#define FB_LIGHT_CYAN    0xB
#define FB_LIGHT_RED     0xC
#define FB_LIGHT_MAGENTA 0xD
#define FB_YELLOW        0xE
#define FB_WHITE         0xF

/* 核心写入函数 */
int fb_write(char *buf, uint32_t len);

/* 初始化函数 */
void fb_init(void);

/* 颜色控制 */
void fb_set_color(uint8_t foreground, uint8_t background);
void fb_get_color(uint8_t *foreground, uint8_t *background);

/* 光标控制 */
void fb_set_cursor(uint32_t position);
uint32_t fb_get_cursor(void);
void fb_move_cursor(int32_t offset);

/* 屏幕管理 */
void fb_clear(void);
void fb_clear_line(uint32_t line);
void fb_scroll(void);

/* 便捷函数 */
void fb_putchar(char c);
void fb_puts(char *str);

/* 直接内存访问 */
void fb_direct_write(uint32_t position, char c, uint8_t fg, uint8_t bg);

/*设置硬件光标位置*/
void fb_set_cursor(unsigned int position);

                                                                                                                     
void fb_write_cell(uint32_t pos, char c, uint8_t fg, uint8_t bg);
uint32_t fb_strlen(char *str);



#endif /* FRAMEBUFFER_H */
