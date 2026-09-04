#include "window.h"
#include "ppu.h"

static void translate_frame_data(uint32_t emulator_window_framebuffer[], const uint32_t ppu_framebuffer[]);

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
void draw_frame(Emulator_Window* emulator_window, const PPU* ppu)
{
    // We take the framebuffer from emulator_window and draw the pixels to sdl window
    translate_frame_data(emulator_window->frame_buffer, ppu->framebuffer);
    SDL_UpdateTexture(
        emulator_window->texture,
        NULL, // NULL means update the entire texture
        emulator_window->frame_buffer,
        GB_SCREEN_WIDTH * sizeof(uint32_t) // Pitch: the number of bytes in a row of pixel data
    );

    // We the current renderer target
    SDL_RenderClear(emulator_window->renderer);

    // Copy the texture to the rendering context
    SDL_RenderCopy(emulator_window->renderer, emulator_window->texture, NULL, NULL);

    // Update the screen with the rendering performed
    SDL_RenderPresent(emulator_window->renderer);
}
static void translate_frame_data(uint32_t emulator_window_framebuffer[], const uint32_t ppu_framebuffer[])
{
    for (int i = 0; i < GB_SCREEN_HEIGHT; i++) {
        for (int j = 0; j < GB_SCREEN_WIDTH; j++) {
            emulator_window_framebuffer[j + GB_SCREEN_WIDTH * i] = colours[ppu_framebuffer[j + GB_SCREEN_WIDTH * i]];
        }
    }
}

void destroy_emulator_window(Emulator_Window* emulator_window)
{
    if (emulator_window->texture)
        SDL_DestroyTexture(emulator_window->texture);
    if (emulator_window->renderer)
        SDL_DestroyRenderer(emulator_window->renderer);
    if (emulator_window->window)
        SDL_DestroyWindow(emulator_window->window);

    SDL_Quit();
}
