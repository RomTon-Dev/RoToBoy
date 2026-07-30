#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

typedef enum {
    TIMER_NOTHING,
    TIMER_APU_DIV,
    TIMER_INTERRUPT,
    TIMER_APU_DIV_INTERRUPT
} timer_tick_output;

typedef struct {
    uint8_t DIV; // FF04 - divider register, incremented at a rate of 16384 hz (64 M-cycles)
    uint8_t TIMA; // FF05 - Timer counter, incremented at frequency determined by TAC
    uint8_t TMA; // FF06 - timer modulo
    uint8_t TAC; // FF07 - timer control, contols TIMA

    // Internal state
    int div_state_clock; // increments every tick, resets at 64
    int tima_state_clock; // increments every tick, resets corresponding to TAC
} timer;

uint8_t timer_read(timer* timer, uint16_t address);

void timer_write(timer* timer, uint16_t address, uint8_t value);

timer_tick_output timer_tick(timer* timer);

#endif
