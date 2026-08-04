#include "ppu.h"
#include <stdint.h>
#include <string.h>

void ppu_init(PPU* ppu)
{
    // zero out arrays
    memset(ppu->vram, 0, sizeof(ppu->vram));
    memset(ppu->oam, 0, sizeof(ppu->oam));
    memset(ppu->framebuffer, 0, sizeof(ppu->framebuffer));

    // set default values to hardware variables
    ppu->lcdc = 0x91;
    ppu->stat = 0x85;
    ppu->scy = 0x00;
    ppu->scx = 0x00;
    ppu->ly = 0x00;
    ppu->lyc = 0x00;
    ppu->bgp = 0xFC;
    ppu->obp0 = 0xFF;
    ppu->obp1 = 0xFF;
    ppu->wy = 0x00;
    ppu->wx = 0x00;

    // initialize internal state
    ppu->m_cycle_counter = 0;
    ppu->window_line = 0;
    ppu->current_mode = PPU_MODE_OAM;

    // initialize outputs and interrupts
    ppu->frame_ready = false;
    ppu->request_vblank_interrupt = false;
    ppu->request_stat_interrupt = false;
}

uint8_t ppu_read(PPU* ppu, uint16_t address)
{
    if (address >= 0x8000 && address <= 0x9FFF) {
        // handle VRAM access
        if (ppu->current_mode == PPU_MODE_TRANSFER) {
            // access to VRAM is blocked in this mode
            return 0xFF;
        }
        return ppu->vram[address - 0x8000];
    } else if (address >= 0xFE00 && address <= 0xFE9F) {
        if (ppu->current_mode == PPU_MODE_TRANSFER || ppu->current_mode == PPU_MODE_OAM) {
            // access to OAM is blocked in these modes
            return 0xFF;
        }
        // handle OAM access
        return ppu->oam[address - 0xFE00];
    } else if (address >= 0xFF40 && address <= 0xFF4B) {
        // handle Register access
        switch (address) {
        case 0xFF40:
            return ppu->lcdc;
        case 0xFF41:
            return ppu->stat;
        case 0xFF42:
            return ppu->scy;
        case 0xFF43:
            return ppu->scx;
        case 0xFF44:
            return ppu->ly;
        case 0xFF45:
            return ppu->lyc;
        case 0xFF47:
            return ppu->bgp;
        case 0xFF48:
            return ppu->obp0;
        case 0xFF49:
            return ppu->obp1;
        case 0xFF4A:
            return ppu->wy;
        case 0xFF4B:
            return ppu->wx;
        default:
            break;
        }
    }
    return 0xFF;
}

void ppu_write(PPU* ppu, uint16_t address, uint8_t value)
{
    if (address >= 0x8000 && address <= 0x9FFF) {
        // handle VRAM access
        if (ppu->current_mode == PPU_MODE_TRANSFER) {
            // access to VRAM is blocked in this mode
            return;
        }
        ppu->vram[address - 0x8000] = value;
    } else if (address >= 0xFE00 && address <= 0xFE9F) {
        if (ppu->current_mode == PPU_MODE_TRANSFER || ppu->current_mode == PPU_MODE_OAM) {
            // access to OAM is blocked in these modes
            return;
        }
        // handle OAM access
        ppu->oam[address - 0xFE00] = value;
    } else if (address >= 0xFF40 && address <= 0xFF4B) {
        // handle Register access
        switch (address) {
        case 0xFF40:
            ppu->lcdc = value;
            break;
        case 0xFF41:
            // bits 0, 1, and 2 are read-only
            // bit 7 is always 1
            // keep the old bottom 3 bits
            // take the new top bits (3-6)
            ppu->stat = (value & 0x78) | (ppu->stat & 0x07) | 0x80;
            break;
        case 0xFF42:
            ppu->scy = value;
            break;
        case 0xFF43:
            ppu->scx = value;
            break;
        case 0xFF44:
            // ly is read-only
            // writing to it resets it to 0
            ppu->ly = 0x00;
            break;
        case 0xFF45:
            ppu->lyc = value;
            break;
        case 0xFF47:
            ppu->bgp = value;
            break;
        case 0xFF48:
            ppu->obp0 = value;
            break;
        case 0xFF49:
            ppu->obp1 = value;
            break;
        case 0xFF4A:
            ppu->wy = value;
            break;
        case 0xFF4B:
            ppu->wx = value;
            break;
        default:
            break;
        }
    }
    return;
}

void ppu_tick(PPU* ppu)
{
    // This is where the meat happens
    // check if the LCD is turned on (bit 7 of lcdc)
    if ((ppu->lcdc & 0x80) == 0) {
        // if LCD is off the PPU does not tick
        // reset state so it's ready when turned back on
        ppu->m_cycle_counter = 0;
        ppu->ly = 0;
        ppu->current_mode = PPU_MODE_HBLANK;

        // update the stat register to reflect HBlank mode
        ppu->stat &= 0xFC; // Clear bottom 2 bits
        return;
    }

    ppu->m_cycle_counter++;
    bool mode_changed = false;

    // check if the mode has changed
    switch (ppu->current_mode) {
    case PPU_MODE_OAM:
        if (ppu->m_cycle_counter >= 20) { // takes 20 M-cycles
            ppu->m_cycle_counter -= 20;
            ppu->current_mode = PPU_MODE_TRANSFER;
            mode_changed = true;
        }
        break;
    case PPU_MODE_TRANSFER:
        if (ppu->m_cycle_counter >= 43) { // takes 43 M-cycles
            ppu->m_cycle_counter -= 43;
            ppu->current_mode = PPU_MODE_HBLANK;
            mode_changed = true;

            // this is where the pixels should be drawn to the window
        }
        break;
    case PPU_MODE_HBLANK:
        if (ppu->m_cycle_counter >= 51) { // takes 51 M-cycles
            // HBLANK is when the scanline is complete so move to the next
            ppu->m_cycle_counter -= 51;
            ppu->ly++; // scanline is complete, move down one line

            if (ppu->ly == GB_SCREEN_HEIGHT) { // LY == 144
                ppu->current_mode = PPU_MODE_VBLANK;
                mode_changed = true;

                // trigger vblank interrupt & flag frame ready
                ppu->request_vblank_interrupt = true;
                ppu->frame_ready = true;
            } else {
                ppu->current_mode = PPU_MODE_OAM;
                mode_changed = true;
            }
        }
        break;
    case PPU_MODE_VBLANK: // takes 114 M-cycles per line, for 10 lines
        if (ppu->m_cycle_counter >= PPU_MCYCLES_PER_SCANLINE) {
            ppu->m_cycle_counter -= PPU_MCYCLES_PER_SCANLINE;
            ppu->ly++;

            if (ppu->ly > 153) { // 144 to 153 is 10 lines
                ppu->ly = 0; // Reset back to top of screen
                ppu->current_mode = PPU_MODE_OAM;
                mode_changed = true;
                ppu->window_line = 0; // Reset internal window counter
            }
        }
        break;
    default:
        break;
    }

    // update and check stat register for interrupts
    if (mode_changed) {
        // update the bottom 2 bits of STAT with the new mode
        ppu->stat = (ppu->stat & 0xFC) | ppu->current_mode;

        // check mode-based STAT interrupts
        if (ppu->current_mode == PPU_MODE_HBLANK && (ppu->stat & 0x08)) {
            ppu->request_stat_interrupt = true;
        } else if (ppu->current_mode == PPU_MODE_VBLANK && (ppu->stat & 0x10)) {
            ppu->request_stat_interrupt = true;
        } else if (ppu->current_mode == PPU_MODE_OAM && (ppu->stat & 0x20)) {
            ppu->request_stat_interrupt = true;
        }
    }

    // update the lyc=ly flag
    if (ppu->ly == ppu->lyc) {
        ppu->stat |= 0x04; // set bit 2
        // check if lyc=ly interrupt is enabled
        if (ppu->stat & 0x40) {
            ppu->request_stat_interrupt = true;
        }
    } else {
        ppu->stat &= ~0x04; // clear bit 2
    }
}
