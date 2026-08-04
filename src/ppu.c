#include "ppu.h"
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
