#include "cartridge.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t* get_rom_contents(const char* filepath);

bool cartridge_load(Cartridge* cart, const char* filepath)
{
    if (cart == NULL || filepath == NULL) {
        return false;
    }

    uint8_t* rom_buffer = get_rom_contents(filepath);
    if (rom_buffer == NULL) {
        printf("Failed to get rom contents");
        return false;
    }

    free(rom_buffer);
    return true;
}

static uint8_t* get_rom_contents(const char* filepath)
{
    FILE* file = fopen(filepath, "rb");
    uint8_t* rom_buffer;

    if (file == NULL) {
        printf("Unable to open file %s", filepath);
        return NULL;
    }

    // find size of file
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    if (size < 0) {
        printf("Failed to determine file size\n");
        fclose(file);
        return NULL;
    }
    fseek(file, 0, SEEK_SET);

    // allocate memory for buffer
    if (!(rom_buffer = malloc(size))) {
        printf("Failed to allocate memory\n");
        fclose(file);
        return NULL;
    }

    // read file contents into buffer
    uint8_t bytes_per_element = 1;
    size_t bytes_read = fread(rom_buffer, bytes_per_element, size, file);

    if (bytes_read != size) {
        printf("Warning: Expected to read %zu bytes, but only read %zu\n", size, bytes_read);
    }

    fclose(file);

    return rom_buffer;
}

void cartridge_free(Cartridge* cart)
{
}
