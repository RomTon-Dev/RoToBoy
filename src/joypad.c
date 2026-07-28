#include "joypad.h"

#define DIFFERENT(x, y) ((!(x)) && (y))

typedef struct {
    bool a, b, select, start;
    bool right, left, up, down;
} joypad_state;

static joypad_state state = { 0 };
static uint8_t p1_select = 0xC0; // bits 4 and 5 initially 1 (unselected)

uint8_t joypad_read(void)
{
    uint8_t result = 0xC0 | p1_select; // bits 7 and 6 are 1
    uint8_t inputs = 0x0F; // all buttons 1 by default (not pressed)

    // Action buttons selected
    if ((p1_select & (1 << 5)) == 0) {
        if (state.a)
            inputs &= ~(1 << 0);
        if (state.b)
            inputs &= ~(1 << 1);
        if (state.select)
            inputs &= ~(1 << 2);
        if (state.start)
            inputs &= ~(1 << 3);
    }

    // Direction keys selected)
    if ((p1_select & (1 << 4)) == 0) {
        if (state.right)
            inputs &= ~(1 << 0);
        if (state.left)
            inputs &= ~(1 << 1);
        if (state.up)
            inputs &= ~(1 << 2);
        if (state.down)
            inputs &= ~(1 << 3);
    }

    return result | inputs;
}

void joypad_write(uint8_t value)
{
    p1_select = value & 0x30; // only bits 4 and 5 are writable
}

bool joypad_set_button(GB_Button button, bool pressed)
{
    // prevent opposite d-pad directions being active
    if (pressed) {
        switch (button) {
        case GB_BUTTON_UP:
            state.down = false;
            break;
        case GB_BUTTON_DOWN:
            state.up = false;
            break;
        case GB_BUTTON_LEFT:
            state.right = false;
            break;
        case GB_BUTTON_RIGHT:
            state.left = false;
            break;
        default:
            break;
        }
    }

    uint8_t old_lines = joypad_read() & 0x0F;
    switch (button) {
    case GB_BUTTON_A:
        state.a = pressed;
        break;
    case GB_BUTTON_B:
        state.b = pressed;
        break;
    case GB_BUTTON_SELECT:
        state.select = pressed;
        break;
    case GB_BUTTON_START:
        state.start = pressed;
        break;
    case GB_BUTTON_RIGHT:
        state.right = pressed;
        break;
    case GB_BUTTON_LEFT:
        state.left = pressed;
        break;
    case GB_BUTTON_UP:
        state.up = pressed;
        break;
    case GB_BUTTON_DOWN:
        state.down = pressed;
        break;
    }
    uint8_t new_lines = joypad_read() & 0x0F;
    return (old_lines & ~new_lines) != 0;
}
