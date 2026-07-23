#include "cartridge.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool load_rom_contents(Cartridge* cart, const char* filepath);
static bool load_battery_save(Cartridge* cart, const char* filepath);
static char* get_save_filepath(const char* filepath);

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
    case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
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
    cart->rom_size = 32 * (0x4000) * (1 << cart->rom_data[0x0148]);
    cart->total_rom_banks = 2 << cart->rom_data[0x0148];

    if (cart->eram_enabled) {
        // Initialize defaults to prevent garbage data math
        cart->total_ram_banks = 0;
        cart->eram_size = 0;

        switch (cart->rom_data[0x0149]) {
        case 0x00:
            cart->eram_size = 0;
            break;
        case 0x01:
            cart->eram_size = 2048; // 2 KiB (Does not use full banks)
            break;
        case 0x02:
            cart->total_ram_banks = 1;
            cart->eram_size = 8 * (1 << 10);
            break;
        case 0x03:
            cart->total_ram_banks = 4;
            cart->eram_size = 32 * (1 << 10);
            break;
        case 0x04:
            cart->total_ram_banks = 16;
            cart->eram_size = 128 * (1 << 10);
            break;
        case 0x05:
            cart->total_ram_banks = 8;
            cart->eram_size = 64 * (1 << 10);
            break;
        default:
            printf("Invalid RAM size\n");
            return false;
        }

        if (cart->eram_size > 0) {
            if (!(cart->eram_data = malloc((size_t)cart->eram_size))) {
                printf("Failed to allocate eram memory\n");
                return false;
            }

            // 1. Initialize fresh RAM to 0xFF
            memset(cart->eram_data, 0xFF, cart->eram_size);

            // 2. If it has a battery, overwrite the 0xFFs with the save file
            if (cart->has_battery) {
                load_battery_save(cart, filepath);
            }
        }
    }
    return true;
}

static bool load_battery_save(Cartridge* cart, const char* filepath)
{
    char* save_filepath = get_save_filepath(filepath);
    FILE* save_file = fopen(save_filepath, "rb");
    if (save_file == NULL) {
        return false; // No save file exists yet
    }

    // Read exactly the cartridge's RAM size from the file
    size_t bytes_read = fread(cart->eram_data, 1, cart->eram_size, save_file);

    // If the save file was smaller than expected, pad the rest with 0xFF
    if (bytes_read < cart->eram_size) {
        memset(cart->eram_data + bytes_read, 0xFF, cart->eram_size - bytes_read);
    }

    fclose(save_file);
    free(save_filepath);
    return true;
}

static char* get_save_filepath(const char* filepath)
{
    // The filepath is the path to the cartridge
    // The .sav file should be called <filepath>.sav
    // e.g. if the cartridge is "path/to/rom/file.gb", the sav is "path/to/rom/file.sav"
    size_t filepath_size = 0;
    {
        int i = 0;
        while (filepath[i] != '\0') {
            i++;
        }
        filepath_size = i;
    }

    // filepath_size + 2 because:
    // 1. '.sav' is 1 char longer than '.gb'
    // 2. We need 1 extra byte for the '\0' null terminator
    char* sav_filepath = malloc((filepath_size + 2) * sizeof(char));

    for (size_t i = 0; i < filepath_size - 2; i++) {
        sav_filepath[i] = filepath[i];
    }

    // last 3 chars now sav instead of gb
    sav_filepath[filepath_size - 2] = 's';
    sav_filepath[filepath_size - 1] = 'a';
    sav_filepath[filepath_size] = 'v';
    sav_filepath[filepath_size + 1] = '\0';

    return sav_filepath;
}

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
    free(cart->rom_data);
    free(cart->eram_data);
}

uint8_t cartridge_read(Cartridge* cart, uint16_t address)
{
    if (address <= 0x3FFF) {
        // this is the fixed bank
        return cart->rom_data[address];
    } else if (address >= 0x4000 && address <= 0x7FFF) {
        // each bank is 16 KiB
        // map for bank n is (address - 0x4000) + (n - 1) * (16KiB)
        uint32_t mapped_address = (address - 0x4000) + (cart->current_rom_bank - 1) * (0x4000);
        if (mapped_address < cart->rom_size) {
            return cart->rom_data[mapped_address];
        }
        return 0xFF;
    } else if (address >= 0xA000 && address <= 0xBFFF) {
        // each bank is 8 KiB
        // bank n is (address - 0xA000) + (n * 8 KiB)
        if (cart->eram_enabled && cart->eram_data != NULL) {
            uint32_t mapped_address = (address - 0xA000) + (cart->current_ram_bank * 0x2000);
            if (mapped_address < cart->eram_size) {
                return cart->rom_data[mapped_address];
            }
        }
        return 0xFF;
    }
    return 0xFF;
}
void cartridge_write(Cartridge* cart, uint16_t address, uint8_t value)
{
    return;
}
