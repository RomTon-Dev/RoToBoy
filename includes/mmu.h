#ifndef MMU_H // also sometimes called bus
#define MMU_H
// header file for the MMU (sometimes called bus), which is what the CPU calls when it wants to access memory. It will perform the validation
// and checks and will perform the memory routing according to the memory map.
#include "cartridge.h"
#include <stdint.h>

typedef struct {
    Cartridge cart; // External rom cartridge

    // Memory located on chip (not in cartridge). Possibly move VRAM to PPU struct once implimented
    uint8_t vram[0x2000]; // 8 KB Video RAM (0x8000-0x9FFF)
    uint8_t wram[0x2000]; // 8 KB Work RAM (0xC000-0xDFFF)
    uint8_t oam[0xA0]; // 160 Bytes Object Attribute Memory (0xFE00-0xFE9F)
    uint8_t hram[0x7F]; // 127 Bytes High RAM (0xFF80-0xFFFE)

    // Eventually link your PPU, APU, and Joypad structs here too:
    // Interrupt Enable (0xFFFF) and Boot ROM mapping flags can live here
    uint8_t ie_register;
    bool boot_rom_mapped;
} mmu;

// The core API your CPU (and DMA) will use to interact with the world
uint8_t bus_read(mmu* bus, uint16_t address);
// this will take the address, decide which memory it is accessing (consult memory map), and will return the corrresponding data
// using the cartridge interface if neccesary.
void bus_write(mmu* bus, uint16_t address, uint8_t value);
// similar to read but writes instead (if allowed)
#endif
