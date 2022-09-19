#include <stdlib.h>

#include "console.h"
#include "entity.h"
#include "levels.h"

#include "player.h"

extern int cx;
extern int cy;
extern SDL_Texture* fox;
extern int frozen;
extern int hx;
extern int hy;
extern int pressed;
extern int released;
extern int shake_time;

void
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
	p->hp = 7;
	cx = (player.x >> 8) - 120;
	cy = player.y - (48<<8);
}

static int
_handle_map(int x, int y)
{
	int t = flags(tile_at(x,y));
	if (t != -1 && t&4) { hx = x; hy = y; }
	if (t != -1 && t&8) { collect(x, y); }
	return t;
}

void
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
	f |= _handle_map(p->x, p->y + q);
	s = p->dir < 0 ? -(11<<8) : (10<<8);
	f |= _handle_map(p->x + s, p->y + q);
	s = p->dir < 0 ? -(20<<8) : (19<<8);
	f |= _handle_map(p->x + s, p->y + q);
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
	f |= _handle_map(p->x + q, p->y);
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
	f |= _handle_map(p->x + q, p->y);
	f |= _handle_map(p->x + q, p->y + (7<<8));
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
	f |= _handle_map(p->x + q, p->y);
	f |= _handle_map(p->x + q, p->y + (7<<8));
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
		shake_time = 12;
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

void
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
