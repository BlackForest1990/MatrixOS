#include "types.h"
#include "drivers/fb.h"

void kmain(void)
{
    /* 初始化帧缓冲区 */
    fb_init();
    
    /* 设置颜色：绿色文字，黑色背景 */
    fb_set_color(FB_GREEN, FB_BLACK);
}
