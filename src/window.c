#include "window.h"
#include "ppu.h"

void init_emulator_window(Emulator_Window* emulator_window)
{
    // window
    emulator_window->window = SDL_CreateWindow(
        "RoToBoy",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT,
        SDL_WINDOW_RESIZABLE // allow the user to drag the window edges
    );
    // renderer
    emulator_window->renderer = SDL_CreateRenderer(
        emulator_window->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // keep pixels sharp
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    // lock the rendering coordinate system to 160x144
    SDL_RenderSetLogicalSize(emulator_window->renderer, GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
    // texture
    emulator_window->texture = SDL_CreateTexture(
        emulator_window->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);

    // frame_buffer
    // set all pixels to white
    for (int i = 0; i < GB_SCREEN_HEIGHT; i++) {
        for (int j = 0; j < GB_SCREEN_WIDTH; j++) {
            emulator_window->frame_buffer[i * GB_SCREEN_WIDTH + j] = colours[WHITE];
        }
    }
}
void draw_frame(const Emulator_Window* emulator_window);
