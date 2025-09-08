// kernel/interrupts/pic.h
#ifndef PIC_H
#define PIC_H

#include "types.h"

void pic_remap(int offset1, int offset2);
void pic_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

#endif
