// kernel/interrupts/keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

void keyboard_handler();
void interrupt_handler(void* regs, uint32_t int_no);
void keyboard_init();
#endif
