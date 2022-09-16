#ifdef __ARM_NEON
#define STBI_NEON
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "graphics.h"
#include "console.h"

SDL_Texture *
load_spritesheet(char const * fp)
{
	SDL_Renderer * renderer = cn_screen._renderer;
	SDL_Texture * texture = NULL;
	unsigned char *data;
	int w;
	int h;
	int n;
	int depth = 32;
	int pitch;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	Uint32 rmask = 0xFF000000;
	Uint32 gmask = 0x00FF0000;
	Uint32 bmask = 0x0000FF00;
	Uint32 amask = 0x000000FF;
#else
	Uint32 rmask = 0x000000FF;
	Uint32 gmask = 0x0000FF00;
	Uint32 bmask = 0x00FF0000;
	Uint32 amask = 0xFF000000;
#endif
	data = stbi_load(fp, &w, &h, &n, 4);
	pitch = 4 * w;
	if (!data)
	{
		return NULL;
	}
	SDL_Surface * surf =
		SDL_CreateRGBSurfaceFrom((void*)(data), w, h,
		                         depth, pitch,
		                         rmask, gmask, bmask, amask);
	if (surf)
	{
		texture = SDL_CreateTextureFromSurface(renderer, surf);
		SDL_FreeSurface(surf);
	}
	stbi_image_free(data);
	return texture;
}
