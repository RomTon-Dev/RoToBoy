#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define TIMER_TICK_NONE 0 // 0000
#define TIMER_TICK_APU_DIV 1 // 0001
#define TIMER_TICK_INTERRUPT 2 // 0010

typedef struct {
    uint16_t system_counter; // increments every M-cycle. DIV (FF04) is the top 8 bits of system_counter
    uint8_t TIMA; // FF05 - Timer counter, incremented at frequency determined by TAC
    uint8_t TMA; // FF06 - timer modulo
    uint8_t TAC; // FF07 - timer control, contols TIMA

} timer;

uint8_t timer_read(timer* timer, uint16_t address);

uint8_t timer_write(timer* timer, uint16_t address, uint8_t value);
// keep in mind writing to DIV resets the system counter, which may also increment TIMA as a falling edge may be detected
// Changing which bit of the system counter is selected (by changing the “Clock select” bits of TAC) from a bit currently set to another that
// is currently unset, will send a “Timer tick” pulse.
// returns any relevent bitflags defined above

uint8_t timer_tick(timer* timer);
// Increases the system_counter by 4, and changes TIMA according to the values of system_counter, TMA and TAC
// returns any relevent bitflags defined above

#endif
