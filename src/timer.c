#include "timer.h"
#include <stdio.h>

#define APU_DIV_BIT (1 << 12) // Bit 4 of DIV = bit 12 of system_counter
#define DIV ((timer->system_counter) >> 8)
#define TAC_FREQ ((timer->TAC) & 0x03) // bottom 2 bits of TAC
#define TAC_ENABLE ((timer->TAC) & 0x04) // bit 2 of TAC

static void tima_tick(timer* timer);
// performs a "timer tick" event and returns corresponding bitflags
static uint8_t set_system_counter(timer* timer, uint16_t value);
// sets system counter to value and returns corresponding bitflags

uint8_t timer_read(timer* timer, uint16_t address)
{
    switch (address) {
    case 0xFF04:
        return DIV;
    case 0xFF05:
        return timer->TIMA;
    case 0xFF06:
        return timer->TMA;
    case 0xFF07:
        return timer->TAC | 0xF8; // top 5 bits are 1
    }
    fprintf(stderr, "error: timer access at invalid address\n");
    return 0xFF;
}

uint8_t timer_write(timer* timer, uint16_t address, uint8_t value)
{
    return 0xFF;
}

uint8_t timer_tick(timer* timer)
{
    uint8_t result = TIMER_NONE;
    // check if reset is due
    if (timer->tima_reset_delay != 0) {
        timer->tima_reset_delay--;

        if (timer->tima_reset_delay == 0) {
            timer->TIMA = timer->TMA; // reset TIMA
            result |= TIMER_INTERRUPT;
        }
    }

    result |= set_system_counter(timer, timer->system_counter + 4);
    return result;
}

static uint8_t set_system_counter(timer* timer, uint16_t value)
{
    uint8_t result = TIMER_NONE;
    uint16_t prev_counter = timer->system_counter;
    timer->system_counter = value;

    if ((prev_counter & APU_DIV_BIT) != 0 && (value & APU_DIV_BIT) == 0) {
        // falling edge on APU_DIV bit
        result |= TIMER_APU_DIV;
    }

    uint8_t tac_div_bits[4] = { 9, 3, 5, 7 };
    uint16_t tima_tick_bit = 1 << tac_div_bits[TAC_FREQ];
    if ((prev_counter & tima_tick_bit) != 0 && (value & tima_tick_bit) == 0 && TAC_ENABLE != 0) {
        // falling edge on tima_tick_bit
        tima_tick(timer);
    }
    return result;
}

static void tima_tick(timer* timer)
{
    // increment TIMA
    timer->TIMA++;
    // check if overflow has accoured
    if (timer->TIMA == 0) {
        timer->tima_reset_delay = 1;
    }
}
