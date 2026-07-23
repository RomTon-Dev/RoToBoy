#ifndef CPU_H
#define CPU_H

#include "mmu.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    // 8-bit and 16-bit General Purpose Registers paired using unions.
    union {
        struct {
            uint8_t f;
            uint8_t a;
        }; // Note: Game Boy is Little-Endian,
        uint16_t af; // so lower byte (F) comes first in memory.
    };

    union {
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };

    union {
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };

    union {
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };

    // Control Registers
    uint16_t sp; // Stack Pointer
    uint16_t pc; // Program Counter
    uint8_t ir; // Instruction Register

    // Internal CPU State
    bool master_interrupt_enable; // IME flag (controlled by EI and DI instructions)
    uint8_t ime_delay; // used for ei, which takes an M-cycle to take effect
    bool halted; // CPU enters low-power halt state
    bool stopped; // CPU enters very low-power stop state

    // Pointer to the system Bus/MMU so the CPU can read/write memory
    mmu* mmu;
} CPU;

// Core API Functions
void cpu_init(CPU* cpu, mmu* mmu); // Initialise CPU with default state
void cpu_step(CPU* cpu); // Executes 1 instruction

#endif
