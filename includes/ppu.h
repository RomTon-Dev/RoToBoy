#ifndef PPU_H
#define PPU_H

#include <stdbool.h>
#include <stdint.h>

#define GB_SCREEN_WIDTH 160
#define GB_SCREEN_HEIGHT 144
#define PPU_MCYCLES_PER_SCANLINE 114 // 456 T-cycles / 4

typedef enum {
    PPU_MODE_HBLANK = 0,
    PPU_MODE_VBLANK = 1,
    PPU_MODE_OAM = 2,
    PPU_MODE_TRANSFER = 3
} ppu_mode_t;

typedef struct {
    // memory
    uint8_t vram[0x2000]; // 0x8000 - 0x9FFF
    uint8_t oam[0xA0]; // 0xFE00 - 0xFE9F

    // hardware registers
    uint8_t lcdc; // 0xFF40, LCD Control
    uint8_t stat; // 0xFF41, LCD Status
    uint8_t scy; // 0xFF42, Scroll Y
    uint8_t scx; // 0xFF43, Scroll X
    uint8_t ly; // 0xFF44, LCD Y Coordinate
    uint8_t lyc; // 0xFF45, LY Compare
    // DMA (0xFF46) is intentionally omitted here
    // handled by the MMU.
    uint8_t bgp; // 0xFF47, Background Palette Data
    uint8_t obp0; // 0xFF48, Object Palette 0 Data
    uint8_t obp1; // 0xFF49, Object Palette 1 Data
    uint8_t wy; // 0xFF4A, Window Y Coordinate
    uint8_t wx; // 0xFF4B, Window X Coordinate

    // internal Stat
    int m_cycle_counter;
    int window_line;
    ppu_mode_t current_mode;

    // output
    uint32_t framebuffer[GB_SCREEN_WIDTH * GB_SCREEN_HEIGHT];
    bool frame_ready;

    // interrupt requests (The MMU will read and clear these)
    bool request_vblank_interrupt;
    bool request_stat_interrupt;
} PPU;

void ppu_init(PPU* ppu);
uint8_t ppu_tick(PPU* ppu);
uint8_t ppu_read(PPU* ppu, uint16_t address);
void ppu_write(PPU* ppu, uint16_t address, uint8_t value);

#endif // PPU_H
