#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#ifndef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#define FB_FLIP_HORZ 1
#define FB_FLIP_VERT 2

struct FrameBuffer
{
	int width;
	int height;
	SDL_Window *win;
	SDL_Renderer *_renderer;
};

/* fb_init: framebuffer, width, height, title */
void fb_init(struct FrameBuffer* restrict,int,int,char const * restrict);
void fb_show(struct FrameBuffer*);

/* the first argument is always the framebuffer,
 * the last the colour index
 */
void fb_fill(struct FrameBuffer*, int);
/* pixel: x, y */
void fb_pixel(struct FrameBuffer*, int, int, int);
/* line: x1, y1, x2, y2 */
void fb_line(struct FrameBuffer*, int, int, int, int, int);
/* hline: x, y, w */
void fb_hline(struct FrameBuffer*, int, int, int, int);
/* vline: x, y, h */
void fb_vline(struct FrameBuffer*, int, int, int, int);
/* (fill_)rect: x1, y1, w, h */
void fb_fill_rect(struct FrameBuffer*, int, int, int, int, int);
void fb_rect(struct FrameBuffer*, int, int, int, int, int);
/* text: s, x, y */
void fb_text(struct FrameBuffer* restrict, char const * restrict,
             int, int, int);

/* spr: index, w, h, x, y, f */
void fb_spr(struct FrameBuffer* restrict, SDL_Texture * restrict,
            int, int, int, int, int, int);
#endif
