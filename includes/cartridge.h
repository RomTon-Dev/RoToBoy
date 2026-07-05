#ifndef CARTRIDGE_H
#define CARTRIDGE_H
// header file describing a gameboy cartridge. Describes the physical contents, and provides an interface for accessing its data
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t* rom_data; // Complete raw ROM array allocated dynamically
    uint32_t rom_size;

    uint8_t* eram_data; // External RAM (for game saves), if present
    uint32_t eram_size;

    uint8_t mbc_type; // e.g., 0 = ROM Only, 1 = MBC1, 3 = MBC3, etc.
    uint8_t current_rom_bank;
    uint8_t current_ram_bank;
    bool ram_enabled;
    // ... Any other MBC-specific state variables (like banking modes). I haven't fully read up on this yet
} Cartridge;

bool cartridge_load(Cartridge* cart, const char* filepath);
void cartridge_free(Cartridge* cart);

// The MMU will call these when reading/writing to 0x0000-0x7FFF or 0xA000-0xBFFF
uint8_t cartridge_read(Cartridge* cart, uint16_t address);
// this will read an address from the ROM and will return the corresponding value, which will depend on which ROM bank is active
void cartridge_write(Cartridge* cart, uint16_t address, uint8_t value);
// similar to the above, but will write data

#endif
