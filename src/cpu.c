#include "cpu.h"
#include "mmu.h"
#include <stdint.h>
#include <string.h>
#define ADD 0
#define ADC 1
#define SUB 2
#define SBC 3
#define AND 4
#define XOR 5
#define OR 6
#define CP 7
#define Z 0x80
#define N 0x40
#define H 0x20
#define C 0x10

static void execute_block_0(CPU* cpu);
static void execute_block_1(CPU* cpu);
static void execute_block_2(CPU* cpu);
static void execute_block_3(CPU* cpu);
static void execute_cb(CPU* cpu);
static bool handle_interrupts(CPU* cpu);
static void perform_isr(CPU* cpu, uint16_t jump_address, uint16_t return_address);
static uint8_t read_reg8(CPU* cpu, uint8_t index);
static void write_reg8(CPU* cpu, uint8_t value, uint8_t index);
static uint16_t read_reg16(CPU* cpu, uint8_t index);
static void write_reg16(CPU* cpu, uint16_t value, uint8_t index);
static uint16_t read_reg16_mem(CPU* cpu, uint8_t index);
static uint16_t read_reg16_stk(CPU* cpu, uint8_t index);
static void write_reg16_stk(CPU* cpu, uint16_t value, uint8_t index);
static void assign_flag(CPU* cpu, uint8_t flag, bool cond);
static bool check_condition(CPU* cpu, uint8_t cond);
static bool get_flag(CPU* cpu, uint8_t flag);

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

void cpu_step(CPU* cpu)
{
    if (cpu->ime_delay > 0) {
        cpu->ime_delay--;
        if (cpu->ime_delay == 0) {
            cpu->master_interrupt_enable = true;
        }
    }
    if (handle_interrupts(cpu)) {
        return;
    }

    if (cpu->halted) {
        system_tick(cpu->mmu);
        return;
    }

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

static bool handle_interrupts(CPU* cpu)
{
    uint8_t pending_interrputs = cpu->mmu->if_register & cpu->mmu->ie_register & 0x1F;
    if (pending_interrputs != 0) {
        cpu->halted = false; // wake up cpu

        if (!cpu->master_interrupt_enable) {
            return false;
        }

        uint16_t jump_address = 0x0040; // VBlank interrupt handler address
        uint16_t return_address = cpu->pc - 1;
        uint8_t mask = 1;
        static const uint16_t isr_vectors[5] = { 0x0040, 0x0048, 0x0050, 0x0058, 0x0060 };
        for (int i = 0; i < 5; i++) {
            if ((pending_interrputs & mask) != 0) {
                // reset IME and IR bit
                cpu->master_interrupt_enable = false;
                cpu->mmu->if_register &= ~mask;
                perform_isr(cpu, isr_vectors[i], return_address);
                return true;
            }

            mask = mask << 1;
        }
        return false;
    }
    return false;
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

static uint8_t read_reg8(CPU* cpu, uint8_t index)
{
    switch (index) {
    case 0:
        return cpu->b;
    case 1:
        return cpu->c;
    case 2:
        return cpu->d;
    case 3:
        return cpu->e;
    case 4:
        return cpu->h;
    case 5:
        return cpu->l;
    case 6:
        return bus_read(cpu->mmu, cpu->hl, true); // [HL]
    case 7:
        return cpu->a;
    }
    return 0xFF;
}

static void write_reg8(CPU* cpu, uint8_t value, uint8_t index)
{
    switch (index) {
    case 0:
        cpu->b = value;
        break;
    case 1:
        cpu->c = value;
        break;
    case 2:
        cpu->d = value;
        break;
    case 3:
        cpu->e = value;
        break;
    case 4:
        cpu->h = value;
        break;
    case 5:
        cpu->l = value;
        break;
    case 6:
        bus_write(cpu->mmu, cpu->hl, value, true); // [HL]
        break;
    case 7:
        cpu->a = value;
        break;
    }
}

static uint16_t read_reg16(CPU* cpu, uint8_t index)
{
    switch (index) {
    case 0:
        return cpu->bc;
    case 1:
        return cpu->de;
    case 2:
        return cpu->hl;
    case 3:
        return cpu->sp;
    }
    return 0xFFFF;
}
static void write_reg16(CPU* cpu, uint16_t value, uint8_t index)
{
    switch (index) {
    case 0:
        cpu->bc = value;
        break;
    case 1:
        cpu->de = value;
        break;
    case 2:
        cpu->hl = value;
        break;
    case 3:
        cpu->sp = value;
        break;
    }
}

static uint16_t read_reg16_mem(CPU* cpu, uint8_t index)
{
    switch (index) {
    case 0:
        return cpu->bc;
    case 1:
        return cpu->de;
    case 2:
        return cpu->hl++;
    case 3:
        return cpu->hl--;
    }
    return 0xFFFF; // should never reach here
}

static uint16_t read_reg16_stk(CPU* cpu, uint8_t index)
{
    switch (index) {
    case 0:
        return cpu->bc;
    case 1:
        return cpu->de;
    case 2:
        return cpu->hl;
    case 3:
        return cpu->af;
    }
    return 0xFFFF;
}
static void write_reg16_stk(CPU* cpu, uint16_t value, uint8_t index)
{
    switch (index) {
    case 0:
        cpu->bc = value;
        break;
    case 1:
        cpu->de = value;
        break;
    case 2:
        cpu->hl = value;
        break;
    case 3:
        cpu->af = value & 0xFFF0; // bottom nibble hardwaried to 0
        break;
    }
}

static bool get_flag(CPU* cpu, uint8_t flag)
{
    return (cpu->f & flag) != 0;
}

static void assign_flag(CPU* cpu, uint8_t flag, bool condition)
{
    if (condition) {
        cpu->f |= flag; // Set if true
    } else {
        cpu->f &= ~flag; // Unset if false
    }
}

static bool check_condition(CPU* cpu, uint8_t cond)
{
    switch (cond & 0x03) {
    case 0:
        return !(cpu->f & Z); // NZ (Not Zero)
    case 1:
        return (cpu->f & Z); // Z  (Zero)
    case 2:
        return !(cpu->f & C); // NC (No Carry)
    case 3:
        return (cpu->f & C); // C  (Carry)
    }
    return false;
}

static void execute_alu_operation(CPU* cpu, uint8_t operation, uint8_t operand)
{
    switch (operation) {
    case ADD: {
        uint16_t result = cpu->a + operand;
        assign_flag(cpu, Z, (result & 0xFF) == 0);
        assign_flag(cpu, N, false);
        assign_flag(cpu, H, ((cpu->a & 0x0F) + (operand & 0x0F)) > 0x0F); // is there a carry for the 5th bit
        assign_flag(cpu, C, result > 0xFF);
        cpu->a = result & 0xFF;
        break;
    }
    case ADC: {
        int c = (cpu->f & C) >> 4;
        int a = cpu->a;
        int op = operand;
        int result = a + op + c;
        assign_flag(cpu, Z, (result & 0xFF) == 0);
        assign_flag(cpu, N, false);
        assign_flag(cpu, H, (a & 0x0F) + (op & 0x0F) + c > 0x0F); // is there a carry for the 5th bit
        assign_flag(cpu, C, result > 0xFF);
        cpu->a = (uint8_t)(result & 0xFF);
        break;
    }
    case SUB: {
        uint8_t result = cpu->a - operand;
        assign_flag(cpu, Z, result == 0);
        assign_flag(cpu, N, true);
        assign_flag(cpu, H, (cpu->a & 0x0F) < (operand & 0x0F)); // is there a carry for the 5th bit
        assign_flag(cpu, C, cpu->a < operand);
        cpu->a = result;
        break;
    }
    case SBC: {
        int c = (cpu->f & C) >> 4;
        int a = cpu->a;
        int op = operand;
        int result = a - op - c;
        assign_flag(cpu, Z, (result & 0xFF) == 0);
        assign_flag(cpu, N, true);
        assign_flag(cpu, H, (a & 0x0F) - (op & 0x0F) - c < 0); // is there a carry for the 5th bit
        assign_flag(cpu, C, result < 0);
        cpu->a = (uint8_t)(result & 0xFF);
        break;
    }
    case AND: {
        uint8_t result = cpu->a & operand;
        assign_flag(cpu, Z, result == 0);
        assign_flag(cpu, N, false);
        assign_flag(cpu, H, true);
        assign_flag(cpu, C, false);
        cpu->a = result;
        break;
    }
    case XOR: {
        uint8_t result = cpu->a ^ operand;
        assign_flag(cpu, Z, result == 0);
        assign_flag(cpu, N, false);
        assign_flag(cpu, H, false);
        assign_flag(cpu, C, false);
        cpu->a = result;
        break;
    }
    case OR: {
        uint8_t result = cpu->a | operand;
        assign_flag(cpu, Z, result == 0);
        assign_flag(cpu, N, false);
        assign_flag(cpu, H, false);
        assign_flag(cpu, C, false);
        cpu->a = result;
        break;
    }
    case CP: {
        // same as SUB but only updates flags
        uint8_t result = cpu->a - operand;
        assign_flag(cpu, Z, result == 0);
        assign_flag(cpu, N, true);
        assign_flag(cpu, H, (cpu->a & 0x0F) < (operand & 0x0F)); // is there a carry for the 5th bit
        assign_flag(cpu, C, cpu->a < operand);
        break;
    }
    }
}

static void execute_block_0(CPU* cpu)
{
    uint8_t z = cpu->ir & 0x07; // bits 0, 1, 2
    uint8_t y = (cpu->ir >> 3) & 0x07; // bits 3, 4, 5 (8-bit register index)
    uint8_t p = y >> 1; // 16-bit register index (0=BC, 1=DE, 2=HL, 3=SP/AF)
    uint8_t q = y & 1; // bit 3

    switch (z) {
    case 0:
        switch (y) {
        case 0:
            // NOP
            break;
        case 1: {
            // LD [imm16], SP
            uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
            uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
            uint16_t write_address = (imm_msb << 8) | imm_lsb;
            bus_write(cpu->mmu, write_address, (uint8_t)(cpu->sp & 0xFF), true); // lsb(SP)
            bus_write(cpu->mmu, write_address + 1, (uint8_t)((cpu->sp >> 8) & 0xFF), true); // msb(SP)
            break;
        }
        case 2:
            // STOP
            cpu->stopped = true;
            cpu->pc++; // stop is a 2 byte instruction
            break;
        case 3: {
            // JR imm8 (Unconditional)
            int8_t offset = (int8_t)(bus_read(cpu->mmu, cpu->pc++, true));
            cpu->pc += offset;
            system_tick(cpu->mmu); // takes 1 M-cycle
            break;
        }
        default: {
            // JR cond, imm8 (y = 4: NZ, 5: Z, 6: NC, 7: C)
            int8_t offset = (int8_t)(bus_read(cpu->mmu, cpu->pc++, true));
            bool condition = check_condition(cpu, y);
            if (condition) {
                cpu->pc += offset;
                system_tick(cpu->mmu);
            }
            break;
        }
        }
        break;

    case 1:
        if (q == 0) {
            // LD r16[p], imm16
            uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
            uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
            uint16_t imm16 = (imm_msb << 8) | imm_lsb;
            write_reg16(cpu, imm16, p);

        } else {
            // ADD HL, r16[p]
            uint16_t r16 = read_reg16(cpu, p);
            uint16_t hl = cpu->hl;

            uint32_t result = (uint32_t)hl + r16;
            assign_flag(cpu, H, (hl & 0x0FFF) + (r16 & 0x0FFF) > 0x0FFF);
            assign_flag(cpu, C, result > 0xFFFF);
            assign_flag(cpu, N, false);
            cpu->hl = (uint16_t)result;
            system_tick(cpu->mmu);
        }
        break;

    case 2:
        if (q == 0) {
            // LD [r16mem], A (BC, DE, HL+, HL-)
            uint16_t r16mem = read_reg16_mem(cpu, p);
            bus_write(cpu->mmu, r16mem, cpu->a, true);
        } else {
            // LD A, [r16mem] (BC, DE, HL+, HL-)
            uint16_t r16mem = read_reg16_mem(cpu, p);
            uint8_t src = bus_read(cpu->mmu, r16mem, true);
            cpu->a = src;
        }
        break;

    case 3:
        if (q == 0) {
            // INC r16[p]
            uint16_t r16 = read_reg16(cpu, p);
            write_reg16(cpu, r16 + 1, p);
            system_tick(cpu->mmu);
        } else {
            // DEC r16[p]
            uint16_t r16 = read_reg16(cpu, p);
            write_reg16(cpu, r16 - 1, p);
            system_tick(cpu->mmu);
        }
        break;

    case 4: {
        // INC r8[y]
        uint8_t r8 = read_reg8(cpu, y);
        uint8_t result = r8 + 1;
        write_reg8(cpu, result, y);
        assign_flag(cpu, Z, result == 0);
        assign_flag(cpu, N, false);
        assign_flag(cpu, H, (r8 & 0x0F) + 1 > 0x0F);
        break;
    }

    case 5: {
        // DEC r8[y]
        uint8_t r8 = read_reg8(cpu, y);
        uint8_t result = r8 - 1;
        write_reg8(cpu, result, y);
        assign_flag(cpu, Z, result == 0);
        assign_flag(cpu, N, true);
        assign_flag(cpu, H, (r8 & 0x0F) < 1);
        break;
    }

    case 6: {
        // LD r8[y], imm8
        uint8_t imm8 = bus_read(cpu->mmu, cpu->pc++, true);
        write_reg8(cpu, imm8, y);
        break;
    }

    case 7:
        // Accumulator / Flag Operations
        switch (y) {
        case 0: {
            // RLCA
            bool b7 = cpu->a >= 128;
            uint8_t res = cpu->a;
            res = res << 1;
            if (b7) {
                res++;
            }
            cpu->a = res;
            assign_flag(cpu, Z, false);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b7);
            break;
        }
        case 1: {
            // RRCA
            bool b0 = (cpu->a % 2) == 1;
            uint8_t res = cpu->a;
            res = res >> 1;
            if (b0) {
                res += 128;
            }
            cpu->a = res;
            assign_flag(cpu, Z, false);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b0);
            break;
        }
        case 2: {
            // RLA
            bool b7 = cpu->a >= 128;
            uint8_t res = cpu->a;
            res = res << 1;
            if (get_flag(cpu, C)) {
                res++;
            }
            cpu->a = res;
            assign_flag(cpu, Z, false);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b7);
            break;
        }
        case 3:
            // RRA
            bool b0 = (cpu->a % 2) == 1;
            uint8_t res = cpu->a;
            res = res >> 1;
            if (get_flag(cpu, C)) {
                res += 128;
            }
            cpu->a = res;
            assign_flag(cpu, Z, false);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b0);
            break;
        case 4: {
            // DAA
            uint8_t correction = 0;
            bool set_carry = false;

            // Check if lower nibble needs adjustment (+6 or -6)
            if (get_flag(cpu, H) || (!get_flag(cpu, N) && (cpu->a & 0x0F) > 0x09)) {
                correction |= 0x06;
            }

            // Check if upper nibble needs adjustment (+60 or -60)
            if (get_flag(cpu, C) || (!get_flag(cpu, N) && cpu->a > 0x99)) {
                correction |= 0x60;
                set_carry = true; // Carry will be set if upper nibble overflows
            }

            if (get_flag(cpu, N)) {
                cpu->a -= correction;
            } else {
                cpu->a += correction;
            }

            assign_flag(cpu, Z, cpu->a == 0);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, set_carry);
            break;
        }
        case 5:
            // CPL
            cpu->a = ~cpu->a;
            assign_flag(cpu, N, true);
            assign_flag(cpu, H, true);
            break;
        case 6:
            // SCF
            assign_flag(cpu, C, true);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            break;
        case 7:
            // CCF
            assign_flag(cpu, C, (cpu->f & C) == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            break;
        }
        break;
    }

    // Trailing prefetch
    cpu->ir = bus_read(cpu->mmu, cpu->pc++, true);
}

static void execute_block_1(CPU* cpu)
{
    uint8_t dest = (cpu->ir >> 3) & 0x07; // bits 3, 4, 5
    uint8_t src = (cpu->ir & 0x07); // bits 0, 1, 2
    if (cpu->ir == 0x76) {
        // halt
        cpu->halted = true;
    } else {
        uint8_t value = read_reg8(cpu, src);
        write_reg8(cpu, value, dest);
    }
    cpu->ir = bus_read(cpu->mmu, cpu->pc++, true);
}

static void execute_block_2(CPU* cpu)
{
    // Register alu operations
    uint8_t operand_reg = (cpu->ir & 0x07); // bits 0, 1, 2
    uint8_t operand = read_reg8(cpu, operand_reg);
    uint8_t operation = (cpu->ir >> 3) & 0x07; // bits 3, 4, 5
    execute_alu_operation(cpu, operation, operand);
    cpu->ir = bus_read(cpu->mmu, cpu->pc++, true);
}

static void execute_block_3(CPU* cpu)
{
    uint8_t z = cpu->ir & 0x07; // Bits 0, 1, 2
    uint8_t y = (cpu->ir >> 3) & 0x07; // Bits 3, 4, 5
    uint8_t p = y >> 1; // 16-bit register index
    uint8_t q = y & 1; // Bit 3

    switch (z) {
    case 0:
        if (y < 4) {
            // RET cond (y = 0:NZ, 1:Z, 2:NC, 3:C)
            bool cond = check_condition(cpu, y);
            system_tick(cpu->mmu);
            if (cond) {
                uint8_t stack_lsb = bus_read(cpu->mmu, cpu->sp++, true);
                uint8_t stack_msb = bus_read(cpu->mmu, cpu->sp++, true);
                cpu->pc = (stack_msb << 8) | stack_lsb;
                system_tick(cpu->mmu);
            }
        } else {
            switch (y) {
            case 4: {
                // LDH [imm8], a
                uint8_t imm8 = bus_read(cpu->mmu, cpu->pc++, true);
                uint16_t write_addr = 0xFF00 | imm8;
                bus_write(cpu->mmu, write_addr, cpu->a, true);
                break;
            }
            case 5: {
                // ADD SP, imm8
                int8_t offset = (int8_t)bus_read(cpu->mmu, cpu->pc++, true);
                uint8_t sp_lsb = cpu->sp & 0xFF;
                uint8_t offset_u8 = (uint8_t)offset;

                assign_flag(cpu, Z, false);
                assign_flag(cpu, N, false);
                assign_flag(cpu, H, (sp_lsb & 0x0F) + (offset_u8 & 0x0F) > 0x0F);
                assign_flag(cpu, C, (int)sp_lsb + (int)offset_u8 > 0xFF);

                system_tick(cpu->mmu);
                cpu->sp = cpu->sp + offset;
                system_tick(cpu->mmu);
                break;
            }
            case 6: {
                // LDH A, [imm8]
                uint8_t imm8 = bus_read(cpu->mmu, cpu->pc++, true);
                uint16_t read_addr = 0xFF00 | imm8;
                imm8 = bus_read(cpu->mmu, read_addr, true);
                cpu->a = imm8;
                break;
            }
            case 7: {
                // LD HL, SP+imm8
                int8_t offset = (int8_t)bus_read(cpu->mmu, cpu->pc++, true);
                uint8_t sp_lsb = cpu->sp & 0xFF;
                uint8_t offset_u8 = (uint8_t)offset;

                assign_flag(cpu, Z, false);
                assign_flag(cpu, N, false);
                assign_flag(cpu, H, (sp_lsb & 0x0F) + (offset_u8 & 0x0F) > 0x0F);
                assign_flag(cpu, C, (int)sp_lsb + (int)offset_u8 > 0xFF);
                system_tick(cpu->mmu);

                cpu->hl = cpu->sp + offset;
                break;
            }
            }
        }
        break;

    case 1:
        if (q == 0) {
            // POP r16[p] (p = 0:BC, 1:DE, 2:HL, 3:AF)
            uint8_t stack_lsb = bus_read(cpu->mmu, cpu->sp++, true);
            uint8_t stack_msb = bus_read(cpu->mmu, cpu->sp++, true);
            write_reg16_stk(cpu, (stack_msb << 8) | stack_lsb, p);
        } else {
            switch (p) {
            case 0: {
                // RET
                uint8_t stack_lsb = bus_read(cpu->mmu, cpu->sp++, true);
                uint8_t stack_msb = bus_read(cpu->mmu, cpu->sp++, true);
                cpu->pc = (stack_msb << 8) | stack_lsb;
                system_tick(cpu->mmu);
                break;
            }
            case 1: {
                // RETI
                uint8_t stack_lsb = bus_read(cpu->mmu, cpu->sp++, true);
                uint8_t stack_msb = bus_read(cpu->mmu, cpu->sp++, true);
                cpu->pc = (stack_msb << 8) | stack_lsb;
                cpu->master_interrupt_enable = true;
                system_tick(cpu->mmu);
                break;
            }
            case 2:
                // JP HL
                cpu->pc = cpu->hl;
                break;
            case 3: // LD SP, HL
                cpu->sp = cpu->hl;
                system_tick(cpu->mmu);
                break;
            }
        }
        break;

    case 2:
        // Conditional Jumps and High RAM (C register)
        if (y < 4) {
            // JP cond, imm16 (y = 0:NZ, 1:Z, 2:NC, 3:C)
            uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
            uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
            bool cond = check_condition(cpu, y);
            if (cond) {
                cpu->pc = (imm_msb << 8) | imm_lsb;
                system_tick(cpu->mmu);
            }
        } else {
            switch (y) {
            case 4:
                // LDH [C], A
                bus_write(cpu->mmu, (0xFF00 | cpu->c), cpu->a, true);
                break;
            case 5: {
                // LD [imm16], A
                uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
                uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
                bus_write(cpu->mmu, (imm_msb << 8) | imm_lsb, cpu->a, true);
                break;
            }
            case 6: {
                // LDH A, [C]
                uint8_t src = bus_read(cpu->mmu, (0xFF00 | cpu->c), true);
                cpu->a = src;
                break;
            }
            case 7: {
                // LD A, [imm16]
                uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
                uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
                cpu->a = bus_read(cpu->mmu, (imm_msb << 8) | imm_lsb, true);
                break;
            }
            }
        }
        break;

    case 3:
        // Unconditional Jumps and Interrupts
        switch (y) {
        case 0: {
            // JP imm16
            uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
            uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
            cpu->pc = (imm_msb << 8) | imm_lsb;
            system_tick(cpu->mmu);
            break;
        }
        case 6:
            // DI (Disable Interrupts)
            cpu->master_interrupt_enable = false;
            cpu->ime_delay = 0; // cancel effect of ei
            break;

        case 7: // EI (Enable Interrupts)
            cpu->ime_delay = 2;
            break;
        default:
            // ILLEGAL opcodes
            break;
        }
        break;

    case 4:
        if (y < 4) {
            // CALL cond, imm16 (y = 0:NZ, 1:Z, 2:NC, 3:C)
            uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
            uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
            if (check_condition(cpu, y)) {
                cpu->sp--;
                system_tick(cpu->mmu);
                bus_write(cpu->mmu, cpu->sp, (cpu->pc & 0xFF00) >> 8, true);
                cpu->sp--;
                bus_write(cpu->mmu, cpu->sp, (cpu->pc & 0x00FF), true);
                cpu->pc = (imm_msb << 8) | imm_lsb;
            }
        } else {
            // ILLEGAL opcodes
        }
        break;

    case 5:
        if (q == 0) {
            // PUSH r16[p] (p = 0:BC, 1:DE, 2:HL, 3:AF)
            cpu->sp--;
            system_tick(cpu->mmu);
            uint16_t r16 = read_reg16_stk(cpu, p);
            bus_write(cpu->mmu, cpu->sp, (r16 & 0xFF00) >> 8, true);
            cpu->sp--;
            bus_write(cpu->mmu, cpu->sp, (r16 & 0x00FF), true);
        } else if (p == 0) {
            // CALL imm16
            uint8_t imm_lsb = bus_read(cpu->mmu, cpu->pc++, true);
            uint8_t imm_msb = bus_read(cpu->mmu, cpu->pc++, true);
            cpu->sp--;
            system_tick(cpu->mmu);
            bus_write(cpu->mmu, cpu->sp, (cpu->pc & 0xFF00) >> 8, true);
            cpu->sp--;
            bus_write(cpu->mmu, cpu->sp, (cpu->pc & 0x00FF), true);
            cpu->pc = (imm_msb << 8) | imm_lsb;
        } else {
            // ILLEGAL opcodes
        }
        break;

    case 6: {
        // ALU immediate operations
        uint8_t imm8 = bus_read(cpu->mmu, cpu->pc++, true);
        execute_alu_operation(cpu, y, imm8);
        break;
    }

    case 7:
        // RST
        cpu->sp--;
        system_tick(cpu->mmu);
        bus_write(cpu->mmu, cpu->sp, (cpu->pc & 0xFF00) >> 8, true);
        cpu->sp--;
        bus_write(cpu->mmu, cpu->sp, (cpu->pc & 0x00FF), true);
        cpu->pc = y * 8;
        break;
    }
    cpu->ir = bus_read(cpu->mmu, cpu->pc++, true);
}

static void execute_cb(CPU* cpu)
{
    uint8_t z = cpu->ir & 0x07; // Bits 0, 1, 2 (r8)
    uint8_t y = (cpu->ir >> 3) & 0x07; // Bits 3, 4, 5 (b3)
    uint8_t x = (cpu->ir >> 6) & 0x03; // bits 6, 7

    switch (x) {
    case 0:
        // bit shifts
        switch (y) {
        case 0: {
            // rlc r8[z]
            uint8_t r8 = read_reg8(cpu, z);
            bool b7 = r8 >= 128;
            uint8_t res = r8;
            res = res << 1;
            if (b7) {
                res++;
            }
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b7);
            break;
        }
        case 1: {
            // rrc r8
            uint8_t r8 = read_reg8(cpu, z);
            bool b0 = (r8 % 2) == 1;
            uint8_t res = r8;
            res = res >> 1;
            if (b0) {
                res += 128;
            }
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b0);
            break;
        }
        case 2: {
            // rl r8
            uint8_t r8 = read_reg8(cpu, z);
            bool b7 = r8 >= 128;
            uint8_t res = r8;
            res = res << 1;
            if (get_flag(cpu, C)) {
                res++;
            }
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b7);
            break;
        }
        case 3: {
            // rr r8
            uint8_t r8 = read_reg8(cpu, z);
            bool b0 = (r8 % 2) == 1;
            uint8_t res = r8;
            res = res >> 1;
            if (get_flag(cpu, C)) {
                res += 128;
            }
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b0);
            break;
        }
        case 4: {
            // sla r8
            uint8_t r8 = read_reg8(cpu, z);
            bool b7 = r8 >= 128;
            uint8_t res = r8;
            res = res << 1;
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b7);
            break;
        }
        case 5: {
            // sra r8
            uint8_t r8 = read_reg8(cpu, z);
            bool b0 = (r8 % 2) == 1;
            uint8_t res = (r8 >> 1) | (r8 & 0x80); // keep bit 7
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b0);
            break;
        }
        case 6: {
            // swap r8
            uint8_t r8 = read_reg8(cpu, z);
            uint8_t lower_nibble = r8 & 0x0F;
            uint8_t upper_nibble = (r8 >> 4) & 0x0F;
            uint8_t res = (lower_nibble << 4) | upper_nibble;
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, false);
            break;
        }
        case 7: {
            // srl r8
            uint8_t r8 = read_reg8(cpu, z);
            bool b0 = (r8 % 2) == 1;
            uint8_t res = r8;
            res = res >> 1;
            write_reg8(cpu, res, z);
            assign_flag(cpu, Z, res == 0);
            assign_flag(cpu, N, false);
            assign_flag(cpu, H, false);
            assign_flag(cpu, C, b0);
            break;
        }
        }
        break;
    case 1: {
        // bit b3[y], r8[z]
        uint8_t r8 = read_reg8(cpu, z);
        uint8_t mask = 1 << y;
        assign_flag(cpu, N, false);
        assign_flag(cpu, H, true);
        assign_flag(cpu, Z, (r8 & mask) == 0);
        break;
    }
    case 2: {
        // res b3, r8
        uint8_t r8 = read_reg8(cpu, z);
        uint8_t mask = ~(1 << y);
        write_reg8(cpu, r8 & mask, z);
        break;
    }
    case 3: {
        // set b3, r8
        uint8_t r8 = read_reg8(cpu, z);
        uint8_t mask = 1 << y;
        write_reg8(cpu, r8 | mask, z);
        break;
    }
    }
    cpu->ir = bus_read(cpu->mmu, cpu->pc++, true);
}
