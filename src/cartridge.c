#include "cartridge.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool load_rom_contents(Cartridge* cart, const char* filepath);

bool cartridge_load(Cartridge* cart, const char* filepath)
{
    if (cart == NULL || filepath == NULL) {
        return false;
    }

    if (!load_rom_contents(cart, filepath)) {
        printf("Failed to get rom contents");
        return false;
    }

    return true;
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
        printf("Failed to allocate memory\n");
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
