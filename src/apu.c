#include "apu.h"

static void tick_sound_lengths(apu* apu);
static void tick_freq_sweep(apu* apu);
static void tick_envelope_sweep(apu* apu);
static void tick_channel_1(apu* apu);
static void tick_channel_2(apu* apu);
static void tick_channel_3(apu* apu);
static void tick_channel_4(apu* apu);
static void trigger_channel(apu* apu, uint8_t channel_no);

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
// REMEMBER TO RELOAD INTERNAL PACE IF OLD PACE WAS 0 FOR CHANNEL 1

void apu_pop_from_buffer(apu* apu, int16_t* out_left, int16_t out_right);
// reads from the head, and advances the head pointer by 2

void apu_div_tick(apu* apu)
{
    apu->div_apu_ticks++;
    switch (apu->div_apu_ticks % 8) {
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

static void tick_channel_1(apu* apu)
{
    // increment period divider, if it overflows, increment duty cycle index
    if (++apu->channel_1.period_divider > 0x7FF) {
        // period divider has overflown
        apu->channel_1.period_divider = apu->channel_1.shadow_register;
        // advance duty step
        apu->channel_1.duty_step = (apu->channel_1.duty_step + 1) % 8;
    }
}

static void tick_freq_sweep(apu* apu)
{
    if (!apu->channel_1.sweep_enabled) {
        return;
    }
    uint8_t new_pace = (apu->channel_1.NRx0 & 0x70) >> 4;
    uint8_t step = apu->channel_1.NRx0 & 0x07;
    bool direction = apu->channel_1.NRx0 & 0x08;
    uint16_t offset = apu->channel_1.shadow_register >> step;
    uint16_t new_period = apu->channel_1.shadow_register;
    if (direction) {
        // subtraction
        new_period -= offset;
    } else {
        new_period += offset;
    }

    if (apu->channel_1.internal_pace == 0) {
        if (!direction && new_period > 0x7FF) {
            apu->channel_1.activated = false;
        }
        return;
    }
    // increment the internal sweep timer, and calculate new period if it reaches the pace (bits 6, 5, 4 or NR11)
    apu->channel_1.sweep_timer++;
    if (apu->channel_1.sweep_timer == apu->channel_1.internal_pace) {
        // iteration
        apu->channel_1.sweep_timer = 0;
        // set pace
        apu->channel_1.internal_pace = new_pace;
        // check overflow
        if (new_period > 0x7FF) {
            // overflow
            apu->channel_1.activated = false;
            return;
        }

        // set period bits if step > 0
        if (step > 0) {
            apu->channel_1.shadow_register = new_period;
            apu->channel_1.NRx3 = (uint8_t)(new_period & 0xFF);
            apu->channel_1.NRx4 = (apu->channel_1.NRx4 & 0xF8) | ((new_period >> 8) & 0x07);

            uint16_t lookahead_offset = apu->channel_1.shadow_register >> step;
            uint16_t lookahead_period = apu->channel_1.shadow_register;

            if (direction) {
                lookahead_period -= lookahead_offset;
            } else {
                lookahead_period += lookahead_offset;
            }

            if (!direction && lookahead_period > 0x7FF) {
                apu->channel_1.activated = false;
            }
        }
    }
}

static void tick_channel_2(apu* apu)
{
    // increment period divider, if it overflows, increment duty cycle index
    if (++apu->channel_2.period_divider > 0x7FF) {
        // period divider has overflown
        apu->channel_2.period_divider = ((apu->channel_2.NRx4 & 0x07) << 8) | apu->channel_2.NRx3;
        // advance duty step
        apu->channel_2.duty_step = (apu->channel_2.duty_step + 1) % 8;
    }
}

static void tick_channel_3_step(apu* apu)
{
    // increment period divider, if it overflows, increment sample index
    if (++apu->channel_3.period_divider > 0x7FF) {
        // period divider has overflown
        apu->channel_3.period_divider = ((apu->channel_3.NRx4 & 0x07) << 8) | apu->channel_3.NRx3;
        // advance sample index
        apu->channel_3.sample_index = (apu->channel_3.sample_index + 1) % SAMPLES;
        // load sample into buffer
        if (apu->channel_3.sample_index % 2 == 0) {
            // upper nibble
            apu->channel_3.sample_buffer = apu->channel_3.wave_ram[apu->channel_3.sample_index / 2] >> 4;
        } else {
            // lower nibble
            apu->channel_3.sample_buffer = apu->channel_3.wave_ram[apu->channel_3.sample_index / 2] & 0x0F;
        }
    }
}

static void tick_channel_3(apu* apu)
{
    if (!apu->channel_3.activated)
        return;

    tick_channel_3_step(apu);
    tick_channel_3_step(apu);
}

static uint32_t get_ch4_period(const apu* apu)
{
    static const uint8_t base_divisors[8] = { 2, 4, 8, 12, 16, 20, 24, 28 };

    uint8_t shift = (apu->channel_4.NRx3 >> 4) & 0x0F;
    uint8_t r = apu->channel_4.NRx3 & 0x07;

    return (uint32_t)base_divisors[r] << shift;
}

static void tick_lfsr(apu* apu)
{
    // Calculate feedback bit (XOR bit 0 and bit 1)
    uint16_t lfsr = apu->channel_4.LFSR;
    uint8_t result = (lfsr & 1) ^ ((lfsr >> 1) & 1);

    // Shift LFSR right by 1 and set bit 14 (MSB) to feedback
    lfsr = (lfsr >> 1) | (result << 14);

    // If 7-bit mode is enabled (NR43 bit 3 set), also write feedback to bit 6
    if (apu->channel_4.NRx3 & 0x08) {
        lfsr = (lfsr & ~(1 << 6)) | (result << 6);
    }

    apu->channel_4.LFSR = lfsr;
}

static void tick_channel_4(apu* apu)
{
    uint8_t shift = (apu->channel_4.NRx3 >> 4) & 0x0F;
    if (!apu->channel_4.activated || shift >= 14)
        return;

    if (apu->channel_4.period_divider > 0) {
        apu->channel_4.period_divider--;
    }

    if (apu->channel_4.period_divider == 0) {
        // Reload period divider
        apu->channel_4.period_divider = get_ch4_period(apu);

        // Advance LFSR state
        tick_lfsr(apu);
    }
}
