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
