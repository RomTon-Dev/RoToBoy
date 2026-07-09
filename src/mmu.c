#include "mmu.h"
#include "cartridge.h"
#include <stdio.h>

uint8_t bus_read(mmu* mmu, uint16_t address)
{
    // Cartridge OR Boot ROM access
    if (address < 0x0100) {
        if (mmu->boot_rom_mapped) {
            return mmu->boot_rom[address];
        } else {
            return cartridge_read(mmu->cart, address);
        }
    }

    // Cartridge ROM access
    if (address < 0x8000) {
        return cartridge_read(mmu->cart, address);
    }

    // VRAM access
    if (address < 0xA000) {
        // TODO: forward to PPU
        return 0xFF;
    }

    // External RAM access (Cartridge SRAM)
    if (address < 0xC000) {
        return cartridge_read(mmu->cart, address);
    }

    // Work RAM access
    if (address < 0xE000) {
        return mmu->wram[address - 0xC000];
    }

    // Echo RAM access (Mirrors WRAM)
    if (address < 0xFE00) {
        return mmu->wram[address - 0xE000];
    }

    // OAM access
    if (address < 0xFEA0) {
        // TODO: forward to PPU
        return 0xFF;
    }

    // Unusable / Restricted Memory
    if (address < 0xFF00) {
        return 0xFF; // Usually reads as 0xFF or 0x00 on real hardware
    }

    // IO Registers ($FF00 - $FF7F)
    if (address < 0xFF80) {

        // Joypad
        if (address == 0xFF00) {
            // return joypad_read(&mmu->joypad);
            return 0xFF; // STUB
        }

        // Serial Transfer (unused unless doing multiplayer)
        if (address == 0xFF01 || address == 0xFF02) {
            return 0xFF;
        }

        // Hardware Timer and divider
        if (address >= 0xFF04 && address <= 0xFF07) {
            // return timer_read(&mmu->timer, address);
            return 0xFF; // STUB
        }

        // Interrupt Flag (IF)
        if (address == 0xFF0F) {
            // The top 3 bits of IF are always 1
            return mmu->if_register | 0xE0; // 11100000
        }

        // Audio (APU)
        if (address >= 0xFF10 && address <= 0xFF3F) {
            // return apu_read(&mmu->apu, address);
            return 0xFF; // STUB
        }

        // OAM DMA Register
        if (address == 0xFF46) {
            return mmu->dma_source_high;
        }

        // Graphics (PPU)
        if (address >= 0xFF40 && address <= 0xFF4B) {
            // return ppu_read(&mmu->ppu, address);
            return 0xFF; // STUB
        }

        // Boot ROM Disable Register
        if (address == 0xFF50) {
            // On an original Game Boy, reading this register returns 0xFF
            return 0xFF;
        }

        // Any unmapped IO registers return 0xFF (Open Bus)
        return 0xFF;
    }

    // HRAM access
    if (address < 0xFFFF) {
        return mmu->hram[address - 0xFF80];
    }

    // IE register access
    if (address == 0xFFFF) {
        return mmu->ie_register;
    }

    // Should never be reached because of the bounds above
    fprintf(stderr, "Unhandled memory read at %04X\n", address);
    return 0xFF;
}
