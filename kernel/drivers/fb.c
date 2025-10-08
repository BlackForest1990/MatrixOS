#include "fb.h"
#include "types.h"
#include "io.h"

/* 帧缓冲区基地址 */
static char *fb = (char *)0xC00B8000;

/* 当前状态 */
static uint32_t cursor_pos = 0;
static uint8_t current_fg = FB_LIGHT_GRAY;
static uint8_t current_bg = FB_BLACK;

/* 字符串长度计算 */
uint32_t fb_strlen(char *str)
{
    uint32_t len = 0;
    while (str && str[len] != '\0') {
        len++;
    }
    return len;
}

/* 初始化帧缓冲区 */
void fb_init(void)
{
    fb_clear();
    cursor_pos = 0;
    current_fg = FB_LIGHT_GRAY;
    current_bg = FB_BLACK;
}

/* 核心写入函数 */
int fb_write(char *buf, uint32_t len)
{
    if (!buf || len == 0) {
        return 0;
    }

    for (uint32_t i = 0; i < len; i++) {
        char c = buf[i];
        
        /* 处理特殊字符 */
        if (c == '\n') {
            cursor_pos = (cursor_pos + 80) / 80 * 80;
        } else if (c == '\t') {
            for (int j = 0; j < 4; j++) {
                fb_write_cell(cursor_pos, ' ', current_fg, current_bg);
                cursor_pos++;
            }
        } else if (c == '\r') {
            cursor_pos = cursor_pos / 80 * 80;
        } else if (c == '\b') {
            if (cursor_pos > 0) {
                cursor_pos--;
                fb_write_cell(cursor_pos, ' ', current_fg, current_bg);
            }
        } else {
            fb_write_cell(cursor_pos, c, current_fg, current_bg);
            cursor_pos++;
        }

        if (cursor_pos >= 80 * 25) {
            fb_scroll();
            cursor_pos = 80 * 24;
        }
    }

    fb_set_cursor(cursor_pos);
    return len;
}

/* 内部函数：写入单个单元格 */
void fb_write_cell(uint32_t pos, char c, uint8_t fg, uint8_t bg)
{
    if (pos < 80 * 25) {
        uint32_t index = pos * 2;
        fb[index] = c;
        fb[index + 1] = ((fg & 0x0F) << 4) | (bg & 0x0F);
    }
}

/* 设置颜色 */
void fb_set_color(uint8_t foreground, uint8_t background)
{
    current_fg = foreground & 0x0F;
    current_bg = background & 0x0F;
}

/* 获取当前颜色 */
void fb_get_color(uint8_t *foreground, uint8_t *background)
{
    if (foreground) *foreground = current_fg;
    if (background) *background = current_bg;
}

/* 清屏 */
void fb_clear(void)
{
    for (uint32_t i = 0; i < 80 * 25; i++) {
        fb_write_cell(i, ' ', current_fg, current_bg);
    }
    cursor_pos = 0;
    fb_set_cursor(0);
}

/* 清空特定行 */
void fb_clear_line(uint32_t line)
{
    if (line < 25) {
        uint32_t start = line * 80;
        for (uint32_t i = 0; i < 80; i++) {
            fb_write_cell(start + i, ' ', current_fg, current_bg);
        }
    }
}

/* 屏幕滚动 */
void fb_scroll(void)
{
    for (uint32_t i = 80; i < 80 * 25; i++) {
        uint32_t src_index = i * 2;
        uint32_t dst_index = (i - 80) * 2;
        fb[dst_index] = fb[src_index];
        fb[dst_index + 1] = fb[src_index + 1];
    }
    fb_clear_line(24);
}

/* 便捷函数：输出字符串 */
void fb_puts(char *str)
{
    if (str) {
        uint32_t len = fb_strlen(str);
        len =  len + 1;
        fb_write(str, len);
    }
}

/* 便捷函数：输出单个字符 */
void fb_putchar(char c)
{
    fb_write(&c, 1);
}

/* 直接内存写入 */
void fb_direct_write(uint32_t position, char c, uint8_t fg, uint8_t bg)
{
    if (position < 80 * 25) {
        fb_write_cell(position, c, fg, bg);
    }
}

/* 设置硬件光标位置 */
void fb_set_cursor(unsigned int position)
{
    /* 确保位置在有效范围内 */
    if (position >= 80 * 25) {
        position = 80 * 25 - 1;
    }
    
    /* 通过I/O端口设置光标位置 */
    outb(0x3D4, 0x0F);  // 告诉显卡要设置光标位置的低字节
    outb(0x3D5, (unsigned char)(position & 0xFF));
    
    outb(0x3D4, 0x0E);  // 告诉显卡要设置光标位置的高字节
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}
