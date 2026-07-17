#include "cartridge.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool load_rom_contents(Cartridge* cart, const char* filepath);
static bool load_battery_save(Cartridge* cart, const char* filepath);

bool cartridge_load(Cartridge* cart, const char* filepath)
{
    if (cart == NULL || filepath == NULL) {
        return false;
    }

    if (!load_rom_contents(cart, filepath)) {
        printf("Failed to get rom contents");
        return false;
    }

    // We need to determine the cartridge type next
    uint8_t cartridge_type = cart->rom_data[0x0147];

    // By default, set everything to false / none
    cart->eram_enabled = false;
    cart->mbc_type = MBC_NONE;
    cart->has_battery = false;
    cart->has_rtc = false;
    cart->current_rom_bank = 1; // Bank 0 is fixed, switchable area starts at 1
    cart->current_ram_bank = 0; // Good idea to initialize this too!
    switch (cartridge_type) {
    // Based on this cartridge_type, determine the mbc_type, has_battery, has_rtc and eram_enabled
    case 0x00:
        // ROM ONLY
        break;
    case 0x01:
        // MBC1
        cart->mbc_type = MBC_1;
        break;
    case 0x02:
        // MBC1+RAM
        cart->mbc_type = MBC_1;
        cart->eram_enabled = true;
        break;
    case 0x03:
        // MBC1+RAM+BATTERY
        cart->mbc_type = MBC_1;
        cart->eram_enabled = true;
        cart->has_battery = true;
        break;
    case 0x05:
        // MBC2
        cart->mbc_type = MBC_2;
        break;
    case 0x06:
        // MBC2+BATTERY
        cart->mbc_type = MBC_2;
        cart->has_battery = true;
        break;
    case 0x08:
        // ROM+RAM
        cart->eram_enabled = true;
        break;
    case 0x09:
        // ROM+RAM+BATTERY
        cart->eram_enabled = true;
        cart->has_battery = true;
        break;
    case 0x0B:
    case 0x0C:
    case 0x0D:
        printf("MMM01 is not currently supported.\n");
        return false;
    case 0x0F:
        // MBC3+TIMER+BATTERY
        cart->mbc_type = MBC_3;
        cart->has_rtc = true;
        cart->has_battery = true;
        break;
    case 0x10:
        // MBC3+TIMER+RAM+BATTERY
        cart->mbc_type = MBC_3;
        cart->has_rtc = true;
        cart->eram_enabled = true;
        cart->has_battery = true;
        break;
    case 0x11:
        // MBC3
        cart->mbc_type = MBC_3;
        break;
    case 0x12:
        // mbc3+ram
        cart->mbc_type = MBC_3;
        cart->eram_enabled = true;
        break;
    case 0x13:
        // MBC3+RAM+BATTERY
        cart->mbc_type = MBC_3;
        cart->eram_enabled = true;
        cart->has_battery = true;
        break;
    case 0x19:
        // MBC5
        cart->mbc_type = MBC_5;
        break;
    case 0x1A:
        // MBC5+RAM
        cart->mbc_type = MBC_5;
        cart->eram_enabled = true;
        break;
    case 0x1B:
        // MBC5+RAM+BATTERY
        cart->mbc_type = MBC_5;
        cart->eram_enabled = true;
        cart->has_battery = true;
        break;
    case 0x1C:
        // MBC5+RUMBLE
        cart->mbc_type = MBC_5;
        break;
    case 0x1D:
        // MBC5+RUMBLE+RAM
        cart->mbc_type = MBC_5;
        cart->eram_enabled = true;
        break;
    case 0x1E:
        // MBC5+RUMBLE+RAM+BATTERY
        cart->mbc_type = MBC_5;
        cart->eram_enabled = true;
        cart->has_battery = true;
        break;
    case 0x20:
        // MBC6
    case 0x22:
        // MBC7+SENSOR+RUMBLE+RAM+BATTERY
    case 0xFC:
        // POCKET CAMERA
    case 0xFD:
        // BANDAI TAMA5
    case 0xFE:
        // HuC3
    case 0xFF:
        // HuC1+RAM+BATTERY
        printf("This cartridge type is not currently supported\n");
        return false;
    default:
        // Invalid cartridge_type
        printf("Invalid cartridge type\n");
        return false;
    }

    // Now that we have the cartridge data, we need the rom and ram sizes
    cart->rom_size = 32 * (1 << 10) * (1 << cart->rom_data[0x0148]);
    cart->total_rom_banks = 2 << cart->rom_data[0x0148];

    if (cart->eram_enabled) {
        switch (cart->rom_data[0x149]) {
        case 0x00:
        case 0x01:
            break;
        case 0x02:
            cart->total_ram_banks = 1;
            break;
        case 0x03:
            cart->total_ram_banks = 4;
            break;
        case 0x04:
            cart->total_ram_banks = 16;
            break;
        case 0x05:
            cart->total_ram_banks = 8;
            break;
        default:
            printf("Invalid RAM size\n");
            return false;
            break;
        }
        cart->eram_size = cart->total_ram_banks * 8 * (1 << 10);
        if (!(cart->eram_data = malloc((size_t)cart->eram_size))) {
            printf("Failed to allocate eram memory\n");
            return false;
        }
        memset(cart->eram_data, 0xFF, cart->eram_size); // Initialize eram data to 0xFF
    }

    return true;
}

// TODO: Implement load_battery_save(Cartridge* cart, const char* filepath)
static bool load_rom_contents(Cartridge* cart, const char* filepath)
{
    FILE* file = fopen(filepath, "rb");

    if (file == NULL) {
        printf("Unable to open file %s", filepath);
        return false;
    }

    // find size of file
    fseek(file, 0, SEEK_END);

    int64_t size = ftell(file);
    if (size < 0) {
        printf("Failed to determine file size\n");
        cart->rom_size = 0;
        fclose(file);
        return false;
    } else if (size > UINT32_MAX) {
        printf("File too large to be valid ROM\n");
        cart->rom_size = 0;
        fclose(file);
        return false;
    }
    cart->rom_size = (uint32_t)size;
    fseek(file, 0, SEEK_SET);

    // allocate memory for buffer
    if (!(cart->rom_data = malloc((size_t)cart->rom_size))) {
        printf("Failed to allocate rom memory\n");
        fclose(file);
        return false;
    }

    // read file contents into buffer
    uint8_t bytes_per_element = 1;
    size_t bytes_read = fread(cart->rom_data, bytes_per_element, (size_t)cart->rom_size, file);

    if (bytes_read != (size_t)cart->rom_size) {
        printf("Warning: Expected to read %zu bytes, but only read %zu\n", (size_t)cart->rom_size, bytes_read);
    }

    fclose(file);

    return true;
}

void cartridge_free(Cartridge* cart)
{
}
