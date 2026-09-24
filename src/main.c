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

static void handle_events(mmu* system_mmu, bool* is_running)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            *is_running = false;
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            if (event.key.repeat) {
                continue;
            }

            bool is_pressed = (event.type == SDL_KEYDOWN);
            bool request_interrupt = false;

            switch (event.key.keysym.sym) {
            case SDLK_UP:
                request_interrupt = joypad_set_button(GB_BUTTON_UP, is_pressed);
                break;
            case SDLK_DOWN:
                request_interrupt = joypad_set_button(GB_BUTTON_DOWN, is_pressed);
                break;
            case SDLK_LEFT:
                request_interrupt = joypad_set_button(GB_BUTTON_LEFT, is_pressed);
                break;
            case SDLK_RIGHT:
                request_interrupt = joypad_set_button(GB_BUTTON_RIGHT, is_pressed);
                break;
            case SDLK_z:
                request_interrupt = joypad_set_button(GB_BUTTON_B, is_pressed);
                break;
            case SDLK_x:
                request_interrupt = joypad_set_button(GB_BUTTON_A, is_pressed);
                break;
            case SDLK_RETURN:
                request_interrupt = joypad_set_button(GB_BUTTON_START, is_pressed);
                break;
            case SDLK_BACKSPACE:
                request_interrupt = joypad_set_button(GB_BUTTON_SELECT, is_pressed);
                break;
            default:
                break;
            }

            // If a button was pressed, trigger the Joypad interrupt in the MMU
            if (request_interrupt) {
                system_mmu->if_register |= 0x10;
            }
        }
    }
}

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

    while (is_running) {
        handle_events(&system_mmu, &is_running);

        while (!system_ppu.frame_ready) {
            cpu_step(&system_cpu);
        }

        system_ppu.frame_ready = false;

        draw_frame(&window, &system_ppu);
    }

    write_battery_save(&cart, argv[1]);
    destroy_emulator_window(&window);
    cartridge_free(&cart);

    return EXIT_SUCCESS;
}
