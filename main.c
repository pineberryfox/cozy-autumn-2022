#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "boids2d.h"
#include "console.h"
#include "graphics.h"
#include "levels.h"
#include "player.h"

/* constants */
static int const _shakex[] = {0, 6, 0, -3, 3, -6};
static int const _shakey[] = {3, 6, 0, -6, -3, 0};
#define _shakes (sizeof(_shakex)/sizeof(_shakex[0]))

/* exported */
int cx;
int cy;
SDL_Texture *fox;
int frozen;
int hx;
int hy;
int pressed;
int released;
int shake_time;

/* nonexported */
static struct FrameBuffer *fb;
static struct B2D_Flock hp_flock;
static SDL_Texture *ui;
static int level;
static int ncx;
static int rcx;
static int rcy;
static int cyt;
static int _paused;

static void
cleanup(void)
{
	if (fox) { SDL_DestroyTexture(fox); }
	if (skybox) { SDL_DestroyTexture(skybox); }
	if (tiles) { SDL_DestroyTexture(tiles); }
}

#ifdef __APPLE__
static void
find_resources(void)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef url;
	CFStringRef fpr;
	char const * thepath;
	url = CFBundleCopyBundleURL(bundle);
	fpr = CFURLCopyPath(url);
	thepath = CFStringGetCStringPtr(fpr, kCFStringEncodingUTF8);
	chdir(thepath);
	CFRelease(fpr);
	CFRelease(url);
	url = CFBundleCopyResourcesDirectoryURL(bundle);
	fpr = CFURLCopyPath(url);
	thepath = CFStringGetCStringPtr(fpr, kCFStringEncodingUTF8);
	chdir(thepath);
	CFRelease(fpr);
	CFRelease(url);
}
#endif

static void
init_flock(void)
{
	int i;
	float d;
	init_flock_defaults(&hp_flock);
	hp_flock.num_boids = (player.hp - 1) / 3;
	hp_flock.target.x = player.x / 256.0f;
	hp_flock.target.y = (player.y - (16<<8)) / 256.0f;
	for (i = 0; i < hp_flock.num_boids; ++i)
	{
		d = i * 2*M_PI / hp_flock.num_boids;
		hp_flock.boids[i].pos.x = hp_flock.target.x;
		hp_flock.boids[i].pos.y = hp_flock.target.y;
		hp_flock.boids[i].vel.x = 3.0f * cos(d);
		hp_flock.boids[i].vel.y = 3.0f * sin(d);
		hp_flock.boids[i].data = 2;
	}
}

static void
draw_flock(void)
{
	static int const _colours[] = {11, 2};
	struct B2D_V2 p;
	struct B2D_V2 v;
	float s;
	int i;
	int d;
	for (i = 0; i < hp_flock.num_boids; ++i)
	{
		p = hp_flock.boids[i].pos;
		v = hp_flock.boids[i].vel;
		d = hp_flock.boids[i].data;
		s = hypotf(v.y,v.x);
		if (s)
		{
			v.x /= s;
			v.y /= s;
		}
		fb_fill_rect(&cn_screen,
		             (int)lroundf(p.x - v.x) - cx - 1,
		             (int)(lroundf(p.y - v.y - cy/256.0f) - 1),
		             3,3,
		             (0 <= d && d < 2) ? _colours[d] : 0);
		fb_fill_rect(&cn_screen,
		             (int)lroundf(p.x) - cx - 1,
		             (int)(lroundf(p.y - cy/256.0f) - 1),
		             3,3,
		             (0 <= d && d < 2) ? _colours[d] : 0);
		fb_fill_rect(&cn_screen,
		             (int)lroundf(p.x + v.x) - cx,
		             (int)(lroundf(p.y + v.y - cy/256.0f)),
		             3,3,
		             19);
		fb_pixel(&cn_screen,
		         (int)lroundf(p.x + 2*v.x) - cx + 1,
		         (int)(lroundf(p.y + 2*v.y - cy/256.0f)),
		         0);
	}
}

static void
main_update(void)
{
	static int const _cdist = 4;
	int i;
	hp_flock.target.x = player.x
		+ (player.dir > 0 ? -(16<<8) : 16<<8);
	hp_flock.target.x /= 256.0f;
	hp_flock.target.y = player.y - (16<<8);
	hp_flock.target.y /= 256.0f;
	/* physics frames */
	while (cn_need_physics())
	{
		if (pressed & CN_BTN_START)
		{
			pressed = 0;
			released = 0;
			_paused = !_paused;
		}
		if (_paused) { continue; }
		--shake_time;
		if (shake_time < 0) { shake_time = 0; }
		--frozen;
		if (frozen < 0) { frozen = 0; }
		if (!frozen)
		{
			update_player(&player);
			update_flock(&hp_flock);
			if (!player.hp)
			{
				reset_level();
				rcx = cx;
				rcy = cy;
				init_flock();
			}
		}
		pressed = 0;
		released = 0;
		if ((player.x >> 8) - rcx > 120 + _cdist)
		{
			rcx = (player.x >> 8) - (120 + _cdist);
		}
		if ((player.x >>8) - rcx < 120 - _cdist)
		{
			rcx = (player.x >> 8) - (120 - _cdist);
		}
		if (player.state != 2)
		{
			cyt = player.y - (54<<8);
		}
		if (player.vy > 0 && ((player.y - rcy)>>8) > 96)
		{
			rcy = player.y - (96 << 8);
		}
		static int const _ytrack = 3<<7;
		if (abs(rcy-cyt) < _ytrack)
		{
			rcy = cyt;
		}
		else
		{
			rcy += _ytrack
				* (!!(rcy < cyt) - !!(rcy > cyt));
		}
		clamp_cam(&rcx, &rcy);
	}
	/* actual frames */
	while (rcx > ncx)
	{
		load_column();
		ncx += 16;
	}
	i = cn_buttons;
	cn_update();
	pressed  |= (cn_buttons ^ i) &  cn_buttons;
	released |= (cn_buttons ^ i) & ~cn_buttons;
}

static void
main_draw(void)
{
	int i;
	int t;
	fb_fill(fb, -1);
	fb_spr(&cn_screen, skybox, 0, 32, 32, -8, -48, 0);
	cx = rcx + (shake_time
	            ? _shakex[(shake_time-1) % _shakes]
	            : 0);
	cy = rcy + ((shake_time
	             ? _shakey[(shake_time-1) % _shakes]
	             : 0) << 8);
	clamp_cam(&cx, &cy);
	map(cx, cy>>8);
	draw_player(&player);
	t = player.hp - 1;
	i = 0;
	while (t > 0 && i < 16)
	{
		hp_flock.boids[i].data = t - 1 > 1 ? 1 : t - 1;
		if (i >= hp_flock.num_boids)
		{
			hp_flock.boids[i].pos.x
				= player.x / 256.0f;
			hp_flock.boids[i].pos.y
				= player.y / 256.0f;
			hp_flock.boids[i].vel.x
				= cosf(i/128.0f * M_PI);
			hp_flock.boids[i].vel.y
				= sinf(i/128.0f * M_PI);
		}
		++i;
		t -= 2;
	}
	hp_flock.num_boids = i;
	draw_flock();
	if (frozen)
	{
		fb_spr(&cn_screen, ui, 3, 1, 1,
		       (hx>>8) - cx, (hy - cy) >> 8, 0);
	}
#ifdef DEBUG
	{
		static char buf[16];
		i = snprintf(buf, 16, "%d", cn_get_fps());
		fb_text(&cn_screen, buf, 240 - 8 * i, 0, -1);
		int x = cn_buttons;
		int y = pressed;
		int z = released;
		for (i = 0; i < 8; ++i)
		{
			if (x & 1) { fb_pixel(fb,235-i,10,11); }
			if (y & 1) { fb_pixel(fb,235-i,10,16); }
			if (z & 1) { fb_pixel(fb,235-i,10,3); }
			x >>= 1;
			y >>= 1;
			z >>= 1;
		}
	}
#endif
	if (_paused)
	{
		fb_text(&cn_screen, "PAUSED", 96, 77, 19);
		fb_text(&cn_screen, "PAUSED", 96, 76, 0);
	}
	fb_show(&cn_screen);
}

static void
step(void)
{
	main_update();
	main_draw();
#ifdef __EMSCRIPTEN__
	EM_ASM({fps_counter.innerText = $0;}, cn_get_fps());
#endif
}

int
main(void)
{
#ifdef __APPLE__
	find_resources();
#endif

	cn_init("Cozy Autumn 2022");
	cn_quit_hook = cleanup;
	fb = &cn_screen;
	level = 0;

	fox = load_spritesheet("fox.png");
	ui = load_spritesheet("ui.png");
	load_level(level);
	rcy = cy;
	rcx = cx;
	ncx = 0;
	init_flock();
	cyt = cy;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(step, 0, 1);
#else
	while (1) { step(); }
#endif
	return 0;
}
