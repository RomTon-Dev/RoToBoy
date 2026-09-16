#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "joypad.h"
#include "mmu.h"
#include "ppu.h"
#include "timer.h"
#include "window.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <path_to_rom.gb>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Cartridge cart = { 0 };
    mmu system_mmu = { 0 };
    CPU system_cpu = { 0 };
    PPU system_ppu = { 0 };
    apu system_apu = { 0 };
    timer system_timer = { 0 };
    Emulator_Window window = { 0 };

    if (!cartridge_load(&cart, argv[1])) {
        printf("Failed to load ROM: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    system_mmu.cart = &cart;
    system_mmu.timer = &system_timer;
    system_mmu.ppu = &system_ppu;
    // put the apu here after it is implemented

    ppu_init(&system_ppu);
    apu_init(&system_apu);
    init_emulator_window(&window);

    cpu_init(&system_cpu, &system_mmu);

    bool is_running = true;
    SDL_Event event;

    while (is_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            }
            // Map joypad input here
        }

        while (!system_ppu.frame_ready) {
            cpu_step(&system_cpu);
        }

        system_ppu.frame_ready = false;

        draw_frame(&window, &system_ppu);
    }

    destroy_emulator_window(&window);
    cartridge_free(&cart);

    return EXIT_SUCCESS;
}
