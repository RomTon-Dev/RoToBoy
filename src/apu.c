#include "apu.h"

static void tick_sound_lengths(apu* apu);
static void tick_freq_sweep(apu* apu);
static void tick_envelope_sweep(apu* apu);
static void tick_channel_1(apu* apu);
static void tick_channel_2(apu* apu);
static void tick_channel_3(apu* apu);
static void tick_channel_4(apu* apu);

void apu_init(apu* apu);
// initialise apu with default state

void apu_tick(apu* apu)
{
    tick_channel_1(apu);
    tick_channel_2(apu);
    tick_channel_3(apu);
    tick_channel_4(apu);

    // add left and right samples to total
    apu->sample_accumulator++;

    if (apu->sample_accumulator > 23.77) {
        // add to queue
    }
}

uint8_t apu_read(apu* apu, uint16_t address);
// returns the corresponding sound register assoicated with the address

void apu_write(apu* apu, uint16_t address, uint8_t value);
// wrties to the corresponding sound register the value given

void apu_pop_from_buffer(apu* apu, int16_t* out_left, int16_t out_right);
// reads from the head, and advances the head pointer by 2

void apu_div_tick(apu* apu)
{
    apu->div_apu_ticks++;
    switch (apu->div_apu_ticks) {
    case 0:
        tick_sound_lengths(apu);
        break;
    case 2:
        // Sound length (258Hz)
        tick_sound_lengths(apu);
        tick_freq_sweep(apu);
        break;
    case 4:
        // CH1 freq sweep (128Hz)
        tick_sound_lengths(apu);
        break;
    case 6:
        tick_sound_lengths(apu);
        tick_freq_sweep(apu);
        break;
    case 7:
        // Envelope sweep (64Hz)
        tick_sound_lengths(apu);
        tick_freq_sweep(apu);
        tick_envelope_sweep(apu);
        break;
    default:
        break;
    }
}
