#ifndef GRAPHICS_H
#define GRAPHICS_H
#ifndef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

SDL_Texture * load_spritesheet(char const *);
#endif
