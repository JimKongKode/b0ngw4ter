#ifndef DISPLAY_INCLUDE
#define DISPLAY_INCLUDE
#include <stdio.h>
#include <SDL2/SDL.h>
typedef struct {
    uint8_t vram[0x2000];
    SDL_Renderer *renderer;
} gb_display_t;
void drawDisplay(void);
#endif
