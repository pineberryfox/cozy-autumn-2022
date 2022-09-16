#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#endif

#include "console.h"
#include "levels.h"
#include "graphics.h"

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
static SDL_Texture *fox;
static struct Entity player;
static int level;
static int pressed;
static int released;
static int ncx;
static int cx;
static int cy;
static int cyt;

static void
init_player(struct Entity * p)
{
	p->y = (29 * 16) << 8;
	p->x = 28 << 8;
	p->dir = 1;
	p->frame = 0;
	p->frame_hold = 6;
	p->state = 0;
}

static void
update_player(struct Entity * p)
{
	static int const accel = 3<<4;
	static int const vxmax = 3<<8;
	static int const vymax = 8<<8;
	static int const grav = 58;
	static int const vy0 = 1248;
	static int jbuf = 0;
	static int coyote = 0;
	int dx = !!(cn_buttons & CN_BTN_RIGHT)
		- !!(cn_buttons & CN_BTN_LEFT);
	int f;
	int q;
	int s;
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
	if ((released & CN_BTN_A) && p->vy < -750)
	{
		p->vy = -750;
	}
	p->vy += grav;
	if (p->vy > vymax)
	{
		p->vy = vymax;
	}
	p->y += p->vy;
	/* check vertical map-collisions */
	q = p->vy <= 0 ? 0 : 8;
	f = 0;
	f |= flags(tile_at(p->x, p->y + (q<<8)));
	s = p->dir < 0 ? -11 : 10;
	f |= flags(tile_at(p->x + (s<<8), p->y + (q<<8)));
	s = p->dir < 0 ? -20 : 19;
	f |= flags(tile_at(p->x + (s<<8), p->y + (q<<8)));
	/* you can always land on solid (1),
	 * while semisolid (2) requires having previously been
	 * not within it
	 * */
	if (f&1 || (f&2 && p->vy > 0
	            && ((p->y - p->vy + (8<<8))>>12
	                < ((p->y + (8<<8))>>12))))
	{
		p->y  = (p->y + (q<<8))&~0xfff;
		p->y -= p->vy <= 0 ? 1 - (17<<8) : 1;
		p->y -= (q<<8);
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
	q = p->dir < 0 ? -23 : 22;
	f = 0;
	f |= flags(tile_at(p->x + (q<<8), p->y));
	f |= flags(tile_at(p->x + (q<<8), p->y + (7<<8)));
	if (f&1)
	{
		p->x  = (p->x + (q<<8))&~0xfff;
		p->x -= p->dir < 0 ? 1 - (17<<8) : 1;
		p->x -= (q<<8);
		p->vx = 0;
	}
	/* check butt-side horizontal map collisions */
	q = p->dir < 0 ? (3<<8) : -(4<<8);
	f = 0;
	f |= flags(tile_at(p->x + q, p->y));
	f |= flags(tile_at(p->x + q, p->y + (7<<8)));
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
	if (!dx && (abs(p->vx) < 1<<5))
	{
		p->vx = 0;
		if (p->state == 1) { p->state = 0; }
		p->frame = 0;
	}
	/* if sufficiently slow, stop and set state to idle (0) */
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
	fb_spr(&cn_screen, fox, frame, 7, 3,
	       (p->x >> 8) - 7 * 4 - cx, (p->y >> 8) - 3 * 4 - cy, flip);
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
	int i;
	struct FrameBuffer * fb;
	static char buf[16];

#ifdef __APPLE__
	find_resources();
#endif

	cn_init("Cozy Autumn 2022");
	cn_quit_hook = cleanup;
	fb = &cn_screen;

	init_player(&player);
	cx = (player.x >> 8) - 120;
	cy = cyt = (player.y >> 8) - 48;

	fox = load_spritesheet("fox.png");
	snprintf(buf, 16, "skybox%02d.png", level);
	load_level(0);
	ncx = 0;

	while (1)
	{
		/* physics frames */
		while (cn_need_physics())
		{
			update_player(&player);
			pressed = 0;
			released = 0;
			if ((player.x >> 8) - cx > 136)
			{
				cx = (player.x >> 8) - 136;
			}
			if ((player.x >>8) - cx < 104)
			{
				cx = (player.x >> 8) - 104;
			}
			if (player.state != 2)
			{
				cyt = (player.y >> 8) - 48;
			}
			if (player.vy > 0 && (player.y>>8) - cy > 96)
			{
				cy = (player.y >> 8) - 96;
			}
			cy += !!(cy < cyt) - !!(cy > cyt);
			clamp_cam(&cx, &cy);
		}
		/* actual frames */
		while (cx > ncx)
		{
			load_column();
			ncx += 16;
		}
		i = cn_buttons;
		cn_update();
		pressed  |= (cn_buttons ^ i) &  cn_buttons;
		released |= (cn_buttons ^ i) & ~cn_buttons;
		i = snprintf(buf, 16, "%d", cn_get_fps());
		fb_fill(fb, -1);
		fb_spr(&cn_screen, skybox, 0, 32, 32, -8, -48, 0);
		map(cx,cy);
		fb_text(&cn_screen, buf, 240 - 8 * i, 0, -1);
		i = snprintf(buf, 16, "%d",
		             flags(tile_at(player.x + (27<<8),player.y)));
		fb_text(&cn_screen, buf, 120 - 8 * i, 0, -1);
		draw_player(&player);
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
		fb_show(&cn_screen);
	}
	return 0;
}
