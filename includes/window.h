#ifndef WINDOW
#define WINDOW
#include "ppu.h"
#include <SDL2/SDL.h>
#include <stdint.h>

typedef enum { WHITE, LIGHT_GREY, DARK_GREY, BLACK } colour;
// Using ARGB
uint32_t colours[4] = {
    0xFFFFFFFF, // WHITE
    0xFFAAAAAA, // LIGHT_GREY
    0xFF555555, // DARK_GREY
    0xFF000000, // BLACK
};

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint32_t frame_buffer[GB_SCREEN_WIDTH * GB_SCREEN_HEIGHT];
} Emulator_Window;

void init_emulator_window(Emulator_Window *emulator_window);
void draw_frame(const Emulator_Window *emulator_window);
#endif // WINDOW
