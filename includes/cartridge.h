#ifndef CARTRIDGE_H
#define CARTRIDGE_H
// header file describing a gameboy cartridge. Describes the physical contents, and provides an interface for accessing its data
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MBC_NONE = 0,
    MBC_1,
    MBC_2,
    MBC_3,
    MBC_5,
} mbc_type_t;

typedef struct {
    uint8_t* rom_data; // Complete raw ROM array allocated dynamically
    uint32_t rom_size;
    uint8_t current_rom_bank;
    uint8_t total_rom_banks; // Total number of 8 KiB ROM banks

    uint8_t* eram_data; // External RAM (for game saves), if present
    uint32_t eram_size;
    uint8_t current_ram_bank;
    uint8_t total_ram_banks; // Total number of 8 KiB RAM banks
    bool eram_enabled;
    bool eram_unlocked;

    mbc_type_t mbc_type; // e.g. MBC_NONE, MBC_1, etc
    bool has_battery; // True if RAM needs to be saved to a .sav file
    bool has_rtc; // True if the cart has a Real-Time Clock (MBC3)
    uint8_t banking_mode;

} Cartridge;

bool cartridge_load(Cartridge* cart, const char* filepath);
void cartridge_free(Cartridge* cart);

// The MMU will call these when reading/writing to 0x0000-0x7FFF or 0xA000-0xBFFF
uint8_t cartridge_read(Cartridge* cart, uint16_t address);
// this will read an address from the ROM and will return the corresponding value, which will depend on which ROM bank is active
void cartridge_write(Cartridge* cart, uint16_t address, uint8_t value);
// similar to the above, but will write data

#endif
