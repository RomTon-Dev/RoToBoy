#include "mmu.h"
#include "cartridge.h"
#include <stdint.h>
#include <stdio.h>

uint8_t bus_read(mmu* mmu, uint16_t address, bool is_cpu)
{

    if (mmu->test_mode && mmu->raw_memory) {
        // fprintf(stderr, "\n[DEBUG] Memory access to: 0x%04X | returning: 0x%02X\n", address, mmu->raw_memory[address]);
        return mmu->raw_memory[address];
    } else {
        // fprintf(stderr, "\n[DEBUG] non test memory access\n");
    }

    // If called by the CPU, other hardware components must progress 1 M-cycle
    if (is_cpu) {
        system_tick(mmu);
    }

    // DMA bus lock
    if (mmu->dma_active && address < 0xFF00) {
        return 0x00;
    }

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
        return 0x00;
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
        return 0x00;
    }

    // Unusable / Restricted Memory
    if (address < 0xFF00) {
        return 0x00; // Usually reads as 0xFF or 0x00 on real hardware
    }

    // IO Registers ($FF00 - $FF7F)
    if (address < 0xFF80) {

        // Joypad
        if (address == 0xFF00) {
            // return joypad_read(&mmu->joypad);
            return 0x00; // STUB
        }

        // Serial Transfer (unused unless doing multiplayer)
        if (address == 0xFF01 || address == 0xFF02) {
            return 0x00;
        }

        // Hardware Timer and divider
        if (address >= 0xFF04 && address <= 0xFF07) {
            // return timer_read(&mmu->timer, address);
            return 0x00; // STUB
        }

        // Interrupt Flag (IF)
        if (address == 0xFF0F) {
            // The top 3 bits of IF are always 1
            return mmu->if_register | 0xE0; // 11100000
        }

        // Audio (APU)
        if (address >= 0xFF10 && address <= 0xFF3F) {
            // return apu_read(&mmu->apu, address);
            return 0x00; // STUB
        }

        // OAM DMA Register
        if (address == 0xFF46) {
            return mmu->dma_source_high;
        }

        // Graphics (PPU)
        if (address >= 0xFF40 && address <= 0xFF4B) {
            // return ppu_read(&mmu->ppu, address);
            return 0x00; // STUB
        }

        // Boot ROM Disable Register
        if (address == 0xFF50) {
            // On an original Game Boy, reading this register returns 0xFF
            return 0x00;
        }

        // Any unmapped IO registers return 0xFF (Open Bus)
        return 0x00;
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
    return 0x00;
}

void bus_write(mmu* mmu, uint16_t address, uint8_t value, bool is_cpu)
{
    if (mmu->test_mode && mmu->raw_memory) {
        mmu->raw_memory[address] = value;
    }
    // If called by the CPU, other hardware components must progress 1 M-cycle
    if (is_cpu) {
        system_tick(mmu);
    }

    // DMA bus lock
    if (mmu->dma_active && address < 0xFF00) {
        return;
    }

    // Cartridge ROM (MBC Bank Switching / RAM Enable)
    if (address < 0x8000) {
        cartridge_write(mmu->cart, address, value);
        return;
    }

    // VRAM
    if (address < 0xA000) {
        // TODO: forward to PPU
        return;
    }

    // External RAM (Cartridge SRAM)
    if (address < 0xC000) {
        cartridge_write(mmu->cart, address, value);
        return;
    }

    // Work RAM
    if (address < 0xE000) {
        mmu->wram[address - 0xC000] = value;
        return;
    }

    // Echo RAM (Mirrors WRAM)
    if (address < 0xFE00) {
        mmu->wram[address - 0xE000] = value;
        return;
    }

    // OAM (Sprite Attribute Table)
    if (address < 0xFEA0) {
        // TODO: forward to PPU
        return;
    }

    // Unusable / Restricted Memory
    if (address < 0xFF00) {
        // Writes to this area are completely ignored by the hardware
        return;
    }

    // IO Registers ($FF00 - $FF7F)
    if (address < 0xFF80) {

        // Joypad
        if (address == 0xFF00) {
            // joypad_write(&mmu->joypad, value);
            return;
        }

        // Hardware Timers
        if (address >= 0xFF04 && address <= 0xFF07) {
            // timer_write(&mmu->timer, address, value);
            return;
        }

        // Interrupt Flag (IF)
        if (address == 0xFF0F) {
            // Ensure the top 3 unused bits are always set to 1
            mmu->if_register = value | 0xE0;
            return;
        }

        // Audio (APU)
        if (address >= 0xFF10 && address <= 0xFF3F) {
            // apu_write(&mmu->apu, address, value);
            return;
        }

        // Graphics (PPU)
        if (address >= 0xFF40 && address <= 0xFF4B && address != 0xFF46) {
            // ppu_write(&mmu->ppu, address, value);
            return;
        }

        // OAM DMA Transfer (Native to MMU)
        if (address == 0xFF46) {
            mmu->dma_source_high = value;

            mmu->dma_source_address = value << 8;

            mmu->dma_delay = 2; // dma takes 1-2 M cycles to start

            // start copying process
            mmu->dma_byte = 0;
            mmu->dma_active = true;
            return;
        }

        // Boot ROM Disable Switch (Native to MMU)
        if (address == 0xFF50) {
            // If any non-zero value is written, unmap the boot ROM permanently.
            if (value != 0) {
                mmu->boot_rom_mapped = false;
            }
            return;
        }

        // Unmapped IO registers ignore writes
        return;
    }

    // HRAM
    if (address < 0xFFFF) {
        mmu->hram[address - 0xFF80] = value;
        return;
    }

    // IE Register
    if (address == 0xFFFF) {
        mmu->ie_register = value;
        return;
    }
}

void dma_tick(mmu* mmu)
{
    if (mmu->dma_active == false) {
        return;
    }

    if (mmu->dma_delay > 0) {
        mmu->dma_delay--;
        return;
    }

    uint16_t current_source = mmu->dma_source_address + mmu->dma_byte;

    uint8_t current_byte = bus_read(mmu, current_source, false);

    uint16_t destination = 0xFE00 + mmu->dma_byte;

    bus_write(mmu, destination, current_byte, false);

    // advance state
    mmu->dma_byte++;
    if (mmu->dma_byte >= 160) {
        mmu->dma_active = false;
        mmu->dma_byte = 0;
    }
}
void system_tick(mmu* mmu)
{
    dma_tick(mmu);
    // ppu_tick(mmu->ppu);
    // apu_tick(mmu->apu);
    // timer_tick(mmu->timer);
}
