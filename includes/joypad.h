#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GB_BUTTON_A,
    GB_BUTTON_B,
    GB_BUTTON_SELECT,
    GB_BUTTON_START,
    GB_BUTTON_RIGHT,
    GB_BUTTON_LEFT,
    GB_BUTTON_UP,
    GB_BUTTON_DOWN
} GB_Button;

uint8_t joypad_read(void);
// return the value of the register at $FF00
void joypad_write(uint8_t value);
// write bits 4 and 5 to register $FF00
bool joypad_set_button(GB_Button button, bool pressed);
// register a joypad button press, and reset any other button presses, returns true iff a line goes from 1 -> 0
// i.e. an interrupt is needed

#endif
