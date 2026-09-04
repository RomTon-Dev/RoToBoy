#ifndef APU_H
#define APU_H

#include <stdbool.h>
#include <stdint.h>

#define SAMPLES 32
#define BUFFER_SIZE 4096

typedef struct {
    uint8_t NRx0; // channel specific feature (if present)
    uint8_t NRx1; // controls length timer
    uint8_t NRx2; // controls volume and envelope
    uint8_t NRx3; // controls the period
    uint8_t NRx4; // has the channels trigger and length timer enable bits, as well as any leftover bits of period

    bool activated; // activated by a write to Nrx4's 7th bit
    bool dac_enabled; // true iff (NRx2 & 0xF8) != 0

    uint8_t length_timer;

    uint8_t sweep_timer;
    bool sweep_enabled;
    uint8_t internal_pace;

    uint8_t shadow_register;
    uint16_t period_divider;
    uint8_t duty_step;
} channel1;

typedef struct {
    uint8_t NRx1; // controls length timer
    uint8_t NRx2; // controls volume and envelope
    uint8_t NRx3; // controls the period
    uint8_t NRx4; // has the channels trigger and length timer enable bits, as well as any leftover bits of period

    bool activated;
    bool dac_enabled; // true iff (NRx2 & 0xF8) != 0

    uint8_t length_timer;
    uint16_t period_divider;
    uint8_t duty_step;
} channel2;

typedef struct {
    uint8_t NRx0; // channel specific feature (if present)
    uint8_t NRx1; // controls length timer
    uint8_t NRx2; // controls volume and envelope
    uint8_t NRx3; // controls the period
    uint8_t NRx4; // has the channels trigger and length timer enable bits, as well as any leftover bits of period

    bool activated;
    bool dac_enabled; // true iff (NRx2 & 0xF8) != 0

    uint8_t wave_ram[SAMPLES / 2]; // wave ram, each byte holds two samples
    uint8_t length_timer;

    uint16_t period_divider;
    uint8_t sample_index;
    uint8_t sample_buffer;
} channel3;

typedef struct {
    uint8_t NRx0; // channel specific feature (if present)
    uint8_t NRx1; // controls length timer
    uint8_t NRx2; // controls volume and envelope
    uint8_t NRx3; // controls the period
    uint8_t NRx4; // has the channels trigger and length timer enable bits, as well as any leftover bits of period

    bool activated;
    uint8_t length_timer;

    uint16_t LFSR;
    uint8_t period_divider;
} channel4;

typedef struct {
    uint8_t div_apu_ticks;

    // channels
    channel1 channel_1; // pulse with period sweep
    channel2 channel_2; // pulese
    channel3 channel_3; // wave output
    channel4 channel_4; // noise

    // global control registers
    uint8_t NR50; // FF24 - Master Volume & VIN panning
    uint8_t NR51; // FF25 - Sound Panning
    uint8_t NR52; // FF26 - Audio Master Control

    // fields for audio execution on a pc
    int16_t circular_buffer[BUFFER_SIZE]; // circular queue with audio signals so there are no gaps
    uint16_t buffer_head;
    uint16_t buffer_tail;

    // audio accumulating variables
    double sample_accumulator;
    int32_t left_sample_sum;
    int32_t right_sample_sum;
    uint8_t sample_sum_count;
} apu;

void apu_init(apu* apu);
// initialise apu with default state

void apu_tick(apu* apu);
// ticks the apu by 1 M-cycle, this involves:
// ticking all channel timers
// incrementing sample_accumulator
// pushing the mixed channel outputs to the ring buffer if accumulator > 24

uint8_t apu_read(apu* apu, uint16_t address);
// returns the corresponding sound register assoicated with the address

void apu_write(apu* apu, uint16_t address, uint8_t value);
// wrties to the corresponding sound register the value given

void apu_pop_from_buffer(apu* apu, int16_t* out_left, int16_t out_right);
// reads from the head, and advances the head pointer by 2
void apu_div_tick(apu* apu);
// Steps the internal Frame Sequencer (ticks volume envelopes, sweeps, and length counters)
#endif
