#ifndef MMU_H // also sometimes called bus
#define MMU_H
// header file for the MMU (sometimes called bus), which is what the CPU calls when it wants to access memory. It will perform the validation
// and checks and will perform the memory routing according to the memory map.
#include "cartridge.h"
#include <stdint.h>

typedef struct {
    // EXTERNAL SUB-COMPONENTS (they manage their own IO)

    Cartridge* cart; // External rom cartridge
    // GB-Timer timer;
    // PPU ppu;
    // APU apu;

    // RAW MEMORY BANKS

    uint8_t boot_rom[256]; // Internal boot rom (0x0000 - 0x00FF);
    uint8_t wram[0x2000]; // 8 KB Work RAM (0xC000-0xDFFF)
    uint8_t hram[0x7F]; // 127 Bytes High RAM (0xFF80-0xFFFE)

    // TO BE PUT IN PPU (here for reference):
    // uint8_t vram[0x2000]; // 8 KB Video RAM (0x8000-0x9FFF)
    // uint8_t oam[0xA0]; // 160 Bytes Object Attribute Memory (0xFE00-0xFE9F)

    // NATIVE MMU IO REGISTERS

    bool boot_rom_mapped; // $FF50
    uint8_t dma_source_high; // $FF46
    uint8_t if_register; // $FF0F
    uint8_t ie_register; // $FFFF

    // DMA state variables (To handle DMA transfer with accurate timing)
    bool dma_active;
    uint8_t dma_byte; // how many bytes have been copied (0 to 159)
    uint8_t dma_delay; // simulates hardware setup delay (in M-cycles)
    uint16_t dma_source_address; // starting address to copy from
} mmu;

// The core API the CPU (and DMA) will use to interact with the world
uint8_t bus_read(mmu* mmu, uint16_t address, bool is_cpu);
// this will take the address, decide which memory it is accessing (consult memory map), and will return the corrresponding data
// using the cartridge interface if neccesary.
void bus_write(mmu* mmu, uint16_t address, uint8_t value, bool is_cpu);
// similar to read but writes instead (if allowed)
void dma_tick(mmu* mmu);
// ticks the DMA state if it is active by 1 M-cycle
void system_tick(mmu* mmu);
// ticks the whole system bu 1 M-cycle. Will be called when an M-cycle progresses
#endif
