#include "cpu.h"
#include "mmu.h"
#include <string.h>

void cpu_init(CPU* cpu, mmu* mmu)
{

    cpu->pc = 0x0000; // Start at the Boot ROM
    cpu->master_interrupt_enable = false; // Interrupts disabled
    cpu->halted = false; // CPU starts awake
    cpu->stopped = false;
    cpu->ir = 0; // nop

    cpu->a = 0x00;
    cpu->f = 0x00;
    cpu->b = 0x00;
    cpu->c = 0x00;
    cpu->d = 0x00;
    cpu->e = 0x00;
    cpu->h = 0x00;
    cpu->l = 0x00;
    cpu->sp = 0x0000; // Will be initialized by the Boot ROM

    if (mmu) {
        cpu->mmu = mmu;
        mmu->boot_rom_mapped = true;
        // add initialisation for boot rom (hardcoded)

        memset(mmu->wram, 0, sizeof(mmu->wram));
        memset(mmu->hram, 0, sizeof(mmu->hram));

        // Interrupt registers start empty
        mmu->ie_register = 0x00;
        mmu->if_register = 0xE0; // top 3 bits are always 1

        mmu->dma_active = false;
        mmu->dma_byte = 0;
        mmu->dma_delay = 0;
        mmu->dma_source_high = 0x00;
        mmu->dma_source_address = 0x0000;

        // external hardware:
        mmu->cart = NULL; // initially no cartridge inserted
        // add initalisations for ppu, apu, timer, joypad once implimented
    }
}

static void execute_block_0(CPU* cpu);
static void execute_block_1(CPU* cpu);
static void execute_block_2(CPU* cpu);
static void execute_block_3(CPU* cpu);
static void execute_cb(CPU* cpu);
static void handle_interrupts(CPU* cpu);
static void perform_isr(CPU* cpu, uint16_t jump_address, uint16_t return_address);

void cpu_step(CPU* cpu)
{
    handle_interrupts(cpu);
    // rememer IR already containes the correct opcode fetched in previous execution
    if (cpu->ir == 0xCB) {
        cpu->ir = bus_read(cpu->mmu, cpu->pc++, true);
        execute_cb(cpu);
    } else {
        uint8_t block = (cpu->ir & 0xC0) >> 6; // bits 7 and 8
        switch (block) {
        case 0:
            execute_block_0(cpu);
            break;
        case 1:
            execute_block_1(cpu);
            break;
        case 2:
            execute_block_2(cpu);
            break;
        case 3:
            execute_block_3(cpu);
            break;
        }
    }
}

static void handle_interrupts(CPU* cpu)
{
    uint8_t pending_interrputs = cpu->mmu->if_register & cpu->mmu->ie_register;
    if (pending_interrputs != 0) {
        cpu->halted = false; // wake up cpu

        if (!cpu->master_interrupt_enable) {
            return;
        }

        uint16_t jump_address = 0x0040; // VBlank interrupt handler address
        uint16_t return_address = cpu->pc - 1;
        uint8_t mask = 1;
        for (int i = 0; i < 5; i++) {
            if ((pending_interrputs & mask) != 0) {
                // reset IME and IR bit
                cpu->master_interrupt_enable = false;
                cpu->mmu->if_register &= ~mask;
                perform_isr(cpu, jump_address, return_address);
                break;
            }

            mask = mask << 1;

            switch (i) {
            case 0: // LCD
                jump_address = 0x0048;
                break;
            case 1: // Timer
                jump_address = 0x0050;
                break;
            case 2: // Serial
                jump_address = 0x0058;
                break;
            case 3: // Joypad
                jump_address = 0x0060;
                break;
            }
        }
    }
}

static void perform_isr(CPU* cpu, uint16_t jump_address, uint16_t return_address)
{
    // tick 2 M cycles
    system_tick(cpu->mmu);
    system_tick(cpu->mmu);
    // push return address onto stack (2 M-cycles)
    uint8_t msb = (return_address) >> 8 & 0xFF;
    uint8_t lsb = return_address & 0xFF;
    cpu->sp--;
    bus_write(cpu->mmu, cpu->sp, msb, true);
    cpu->sp--;
    bus_write(cpu->mmu, cpu->sp, lsb, true);
    // jump to jump address
    cpu->pc = jump_address;
    // pre-fetch first instruction
    cpu->ir = bus_read(cpu->mmu, cpu->pc++, true);
}
