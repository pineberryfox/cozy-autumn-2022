#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "console.h"

static SDL_Texture *
load_spritesheet(char const *);

static SDL_Texture *fox;
struct Entity
{
	int x;
	int y;
	int vx;
	int vy;
	int dir;
	int frame;
	int frame_hold;
	int state;
	int turnback_time;
};
static struct Entity player;
static int pressed = 0;
static int released = 0;

static void
init_player(struct Entity * p)
{
	p->y = 128 << 8;
	p->x = 28 << 8;
	p->dir = 1;
	p->frame = 0;
	p->frame_hold = 6;
	p->state = 0;
}

static void
update_player(struct Entity * p)
{
	static int const accel = 1<<6;
	static int const vxmax = 3<<8;
	static int const vymax = 3<<8;
	static int const grav = 58;
	static int const vy0 = 1098;
	static int jbuf = 0;
	static int coyote = 0;
	int dx = !!(cn_buttons & CN_BTN_RIGHT)
		- !!(cn_buttons & CN_BTN_LEFT);
	/* default to jumping/falling (2) state */
	p->state = 2;
	if (pressed & CN_BTN_A) { jbuf = 4; }
	if (--jbuf < 0) { jbuf = 0; }
	if (--coyote < 0) { coyote = 0; }
	if (jbuf && coyote)
	{
		p->vy = -vy0;
		p->turnback_time = 0;
		p->frame = 2;
		jbuf = coyote = 0;
	}
	if ((released & CN_BTN_A) && p->vy < -600)
	{
		p->vy = -600;
	}
	p->vy += grav;
	if (p->vy > vymax)
	{
		p->vy = vymax;
	}
	p->y += p->vy;
	if (p->y > (128<<8))
	{
		p->y = 128<<8;
		coyote = 6;
		p->state = 1;
	}
	/* if grounded, state is running (1), might override to idle */

	if (dx != 0 && p->state != 2)
	{
		if (p->dir != dx)
		{
			p->turnback_time = 12;
		}
		p->dir = dx;
	}
	--(p->turnback_time);
	if (p->turnback_time < 0) { p->turnback_time = 0; }
	p->vx += accel * dx / (p->state == 2 ? 2 : 1);
	if (!dx) { p->vx *= 9; p->vx /= 10; }
	if (abs(p->vx) > vxmax)
	{
		p->vx = p->vx < 0 ? -vxmax : vxmax;
	}
	p->x += p->vx;
	if (p->x > ((240 - 28 - 1) << 8))
	{
		p->x = (240 - 28 - 1) << 8;
		p->vx = 0;
	}
	if (p->x < (28 << 8))
	{
		p->x = 28 << 8;
		p->vx = 0;
	}
	if (p->state != 2 && !--p->frame_hold)
	{
		/* avoid changing frames for jump-state
		 * because we want to smoothly come out of landing.
		 * */
		p->frame_hold = 6;
		++(p->frame);
		p->frame &= 3;
	}
	if (!dx && (abs(p->vx) < 1<<5))
	{
		p->vx = 0;
		if (p->state == 1) { p->state = 0; }
		p->frame = 4;
	}
	/* if sufficiently slow, stop and set state to idle (0) */
}

static void
draw_player(struct Entity * p)
{
	int i = !!(p->turnback_time);
	int flip = ((p->dir < 0) ^ i) ? FB_FLIP_HORZ : 0;
	int frame = 42 * p->frame;
	if (p->state == 2)
	{
		frame = 42 * 5;
		i = p->vy < 0 ? 0 : 1;
	}
	fb_spr(&cn_screen, fox, frame + 7 * i, 7, 3,
	       (p->x >> 8) - 7 * 4, (p->y >> 8) - 3 * 4, flip);
}

int
main(int argc, char **argv)
{
	int i;
	struct FrameBuffer * fb;
	static char buf[16];

	init_player(&player);

	cn_init("Cozy Autumn 2022");
	fb = &cn_screen;

	fox = load_spritesheet("fox.png");

	while (1)
	{
		/* physics frames */
		while (cn_need_physics())
		{
			update_player(&player);
			pressed = 0;
			released = 0;
		}
		/* actual frames */
		i = cn_buttons;
		cn_update();
		pressed  |= (cn_buttons ^ i) &  cn_buttons;
		released |= (cn_buttons ^ i) & ~cn_buttons;
		i = snprintf(buf, 16, "%d", cn_get_fps());
		fb_fill(fb, 11);
		fb_text(&cn_screen, buf, 240 - 8 * i, 0, 0);
		i = snprintf(buf, 16, "%d", player.state);
		fb_text(&cn_screen, buf, 120 - 8 * i, 0, 0);
		draw_player(&player);
		int x = cn_buttons;
		int y = pressed;
		int z = released;
		for (i = 0; i < 8; ++i)
		{
			if (x & 1) { fb_pixel(fb,235-i,10,0); }
			if (y & 1) { fb_pixel(fb,235-i,10,16); }
			if (z & 1) { fb_pixel(fb,235-i,10,3); }
			x >>= 1;
			y >>= 1;
			z >>= 1;
		}
		fb_show(&cn_screen);
	}
	return 0;
}

static SDL_Texture *
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
