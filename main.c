#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#endif

#include "console.h"
#include "levels.h"
#include "graphics.h"

static int const _shakex[] = {0, 6, 0, -3, 3, -6};
static int const _shakey[] = {3, 6, 0, -6, -3, 0};
static int _shake_time;
#define _shakes (sizeof(_shakex)/sizeof(_shakex[0]))

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
	int iframes;
	int hp;
};
static SDL_Texture *fox;
static SDL_Texture *ui;
static struct Entity player;
static int level;
static int pressed;
static int released;
static int ncx;
static int cx;
static int cy;
static int rcx;
static int rcy;
static int cyt;
static int frozen;
static int hx;
static int hy;

static void
init_player(struct Entity * p)
{
	p->x = 28 << 8;
	p->y = (29 * 16) << 8;
	p->vx = 0;
	p->vy = 0;
	p->dir = 1;
	p->frame = 0;
	p->frame_hold = 6;
	p->state = 0;
	p->turnback_time = 0;
	p->iframes = 0;
	p->hp = 10;
	cx = (player.x >> 8) - 120;
	cy = cyt = player.y - (48<<8);
}

static void
update_player(struct Entity * p)
{
	static int const accel = 3<<4;
	static int const vxmax = 3<<8;
	static int const vymax = 8<<8;
	static int const grav = 58;
	static int const vy0 = 1248;
	static int const max_iframes = 60;
	static int const no_ctrl_time = 20;
	static int jbuf = 0;
	static int coyote = 0;
	int hurt = 0;
	int dx = !!(cn_buttons & CN_BTN_RIGHT)
		- !!(cn_buttons & CN_BTN_LEFT);
	int f;
	int q;
	int s;
	int t;
	/* default to jumping/falling (2) state */
	p->state = 2;
	if (pressed & CN_BTN_A) { jbuf = 4; }
	if (--jbuf < 0) { jbuf = 0; }
	if (--coyote < 0) { coyote = 0; }
	if (p->iframes > max_iframes - no_ctrl_time
	    || p->y > ((32 * 16)<<8))
	{
		dx = jbuf = 0;
	}
	if (--(p->iframes) < 0) { p->iframes = 0; }
	if (jbuf && coyote)
	{
		p->vy = -vy0;
		p->turnback_time = 0;
		p->frame = 2;
		jbuf = coyote = 0;
	}
	/*
	if ((released & CN_BTN_A) && p->vy < -750)
	{
		p->vy = -750;
	}
	*/
	if ((released & CN_BTN_A) && p->vy < 0)
	{
		p->vy /= 2;
	}
	p->vy += grav;
	if (p->vy > vymax)
	{
		p->vy = vymax;
	}
	p->y += p->vy;
	/* check vertical map-collisions */
	q = p->vy <= 0 ? 0 : (8<<8);
	f = 0;
	t= flags(tile_at(p->x, p->y + q));
	if (t != -1 && t&4) { hx = p->x; hy = p->y + q; }
	f |= t;
	s = p->dir < 0 ? -(11<<8) : (10<<8);
	t = flags(tile_at(p->x + s, p->y + q));
	if (t != -1 && t&4) { hx = p->x + s; hy = p->y + q; }
	f |= t;
	s = p->dir < 0 ? -(20<<8) : (19<<8);
	t = flags(tile_at(p->x + s, p->y + q));
	if (t != -1 && t&4) { hx = p->x + s; hy = p->y + q; }
	f |= t;
	if (f != -1) { hurt |= f&4; }
	/* you can always land on solid (1),
	 * while semisolid (2) requires having previously been
	 * not within it
	 * */
	if (f&1 || (f&2 && p->vy > 0
	            && ((p->y - p->vy + (8<<8))>>12
	                < ((p->y + (8<<8))>>12))))
	{
		p->y  = (p->y + q)&~0xfff;
		p->y -= p->vy <= 0 ? 1 - (17<<8) : 1;
		p->y -= q;
		if (p->vy > 0)
		{
			coyote = 6;
			p->state = 1;
		}
		p->vy = 0;
	}
	/* if grounded, state is running (1), might override to idle */

	if (dx != 0 && p->state != 2)
	{
		if (p->dir != dx)
		{
			p->turnback_time = p->frame_hold;
			if (p->frame != 3)
			{
				p->turnback_time += 5;
			}
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
	/* check nose-side horizontal map collisions */
	q = p->dir < 0 ? -(23<<8) : (22<<8);
	f = 0;
	t = flags(tile_at(p->x + q, p->y));
	if (t != -1 && t&4) { hx = p->x + q; hy = p->y; }
	f |= t;
	/*
	t = flags(tile_at(p->x + q, p->y + (7<<8)));
	if (t != -1 && t&4) { hx = p->x + q; hy = p->y + (7<<8); }
	f |= t;
	*/
	if (f != -1) { hurt |= f&4; }
	if (f&1)
	{
		p->x  = (p->x + q)&~0xfff;
		p->x -= p->dir < 0 ? 1 - (17<<8) : 1;
		p->x -= q;
		p->vx = 0;
	}
	/* check interior horizontal map collisions */
	q = p->dir < 0 ? -(9<<8) : (9<<8);
	f = 0;
	t = flags(tile_at(p->x + q, p->y));
	if (t != -1 && t&4) { hx = p->x + q; hy = p->y; }
	f |= t;
	t = flags(tile_at(p->x + q, p->y + (7<<8)));
	if (t != -1 && t&4) { hx = p->x + q; hy = p->y + (7<<8); }
	f |= t;
	if (f != -1) { hurt |= f&4; }
	if (f&1)
	{
		p->x  = (p->x + q)&~0xfff;
		p->x -= p->dir < 0 ? 1 - (17<<8) : 1;
		p->x -= q;
		p->vx = 0;
	}
	/* check butt-side horizontal map collisions */
	q = p->dir < 0 ? (3<<8) : -(4<<8);
	f = 0;
	t = flags(tile_at(p->x + q, p->y));
	if (t != -1 && t&4) { hx = p->x + q; hy = p->y; }
	f |= t;
	t = flags(tile_at(p->x + q, p->y + (7<<8)));
	if (t != -1 && t&4) { hx = p->x + q; hy = p->y + (7<<8); }
	f |= t;
	/* take another bit to know direction */
	if (f != -1) { hurt |= (f&4)>>1; }
	if (f&1)
	{
		p->x  = (p->x + q)&~0xfff;
		p->x += p->dir < 0 ? -1 : (16<<8);
		p->x -= q;
		p->vx = 0;
	}
	if (p->state != 2 && !--p->frame_hold)
	{
		/* avoid changing frames for jump-state
		 * because we want to smoothly come out of landing.
		 */
		p->frame_hold = 5;
		++(p->frame);
		p->frame &= 3;
	}
	if (hurt && !p->iframes)
	{
		--(p->hp);
		p->iframes = max_iframes;
		p->vx = (!!(hurt&4) - !!(hurt&2)) * (-p->dir) * (2<<8);
		p->vy = -750;
		frozen = 6;
		_shake_time = 12;
	}
	/* if sufficiently slow, stop and set state to idle (0) */
	if (!dx && (abs(p->vx) < 1<<5))
	{
		p->vx = 0;
		if (p->state == 1) { p->state = 0; }
		p->frame = 0;
	}
	/* void damage */
	if (!p->iframes && p->y > (32 * 16)<<8)
	{
		--(p->hp);
		p->iframes = 5 * (10 - p->hp) / 3;
		if (p->hp < 0) { p->hp = 0; }
	}
}

static void
draw_player(struct Entity * p)
{
	int const i = p->state == 2 ? 0 : !!(p->turnback_time);
	int const flip = ((p->dir < 0) ^ i) ? FB_FLIP_HORZ : 0;
	int const stage = p->state == 2
		? (abs(p->vy) < 100 ? 1 : (p->vy < 0 ? 0 : 2))
		: p->frame;
	int const frame = 32 * 4 * (2 * p->state + i) + 8 * stage;
	if (p->iframes & 4) { return; }
	fb_spr(&cn_screen, fox, frame, 7, 3,
	       (p->x >> 8) - 7 * 4 - cx,
	       ((p->y - cy) >> 8) - 3 * 4, flip);
}

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

int
main(int argc, char **argv)
{
	static int const _cdist = 4;
	int i;
	int t;
	struct FrameBuffer * fb;

#ifdef __APPLE__
	find_resources();
#endif

	cn_init("Cozy Autumn 2022");
	cn_quit_hook = cleanup;
	fb = &cn_screen;
	level = 0;

	init_player(&player);

	fox = load_spritesheet("fox.png");
	ui = load_spritesheet("ui.png");
	load_level(level);
	ncx = 0;

	while (1)
	{
		/* physics frames */
		while (cn_need_physics())
		{
			--_shake_time;
			if (_shake_time < 0) { _shake_time = 0; }
			--frozen;
			if (frozen < 0) { frozen = 0; }
			if (!frozen)
			{
				update_player(&player);
				if (!player.hp)
				{
					init_player(&player);
				}
			}
			pressed = 0;
			released = 0;
			if ((player.x >> 8) - rcx > 120 + _cdist)
			{
				rcx = (player.x >> 8) - (120 + _cdist);
			}
			if ((player.x >>8) - cx < 120 - _cdist)
			{
				rcx = (player.x >> 8) - (120 - _cdist);
			}
			if (player.state != 2)
			{
				cyt = player.y - (54<<8);
			}
			if (player.vy > 0 && ((player.y - cy)>>8) > 96)
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
		fb_fill(fb, -1);
		fb_spr(&cn_screen, skybox, 0, 32, 32, -8, -48, 0);
		cx = rcx + (_shake_time
		            ? _shakex[(_shake_time-1) % _shakes]
		            : 0);
		cy = rcy + ((_shake_time
		             ? _shakey[(_shake_time-1) % _shakes]
		             : 0) << 8);
		clamp_cam(&cx, &cy);
		map(cx, cy>>8);
		draw_player(&player);
		if (frozen)
		{
			fb_spr(&cn_screen, ui, 3, 1, 1,
			       (hx>>8) - cx, (hy - cy) >> 8, 0);
		}
		t = player.hp;
		for(i = 0; i < 5; ++i)
		{
			fb_spr(&cn_screen, ui, (t > 2) ? 2 : t, 1, 1,
			       100 + 8 * i, 4, 0);
			t -= 2;
			if (t < 0) { t = 0; }
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
		fb_show(&cn_screen);
	}
	return 0;
}
