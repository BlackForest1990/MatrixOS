// kernel/interrupts/keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

void interrupt_handler(void* regs, uint32_t int_no);
void keyboard_init();
void keyboard_input_handler(uint8_t sc);
char keyboard_get_char(void);
char scancode_to_char(uint8_t sc);

#endif
