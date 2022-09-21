#include <stdlib.h>

#include "common.h"
#include "console.h"
#include "entity.h"
#include "levels.h"

#include "player.h"

struct Entity player;

static int max_iframes = 64;
static int _update_player(struct Entity *);
static void _draw_player(struct Entity *);
static void _hurt_player(struct Entity *, int);

static void
_hitspark(struct Entity *p, int x, int y)
{
	if (!p) { return; }
	p->hpos.x = x;
	p->hpos.y = y;
	p->hit = 1;
}

void
init_player(struct Entity * p, int x, int y)
{
	if (!p) { return; }
	p->pos.x = x;
	p->pos.y = y;
	p->vel.x = 0;
	p->vel.y = 0;
	p->hpos.x = 0;
	p->hpos.y = 0;
	p->hit = 0;
	p->dir = 1;
	p->frame = 0;
	p->frame_hold = 6;
	p->state = 0;
	p->turnback_time = 0;
	p->iframes = 0;
	p->hp = 7;
	p->update = _update_player;
	p->draw = _draw_player;
	p->hurt = _hurt_player;
	p->hb1.pos.x = 16<<8;
	p->hb1.pos.y = 2<<8;
	p->hb1.radius = 5<<8;
	p->hb2.pos.x = 2<<8;
	p->hb2.pos.y = 6<<8;
	p->hb2.radius = 6<<8;
	p->has_two_spheres = 1;
	cam.x = p->pos.x - (120<<8);
	cam.y = p->pos.y - (48<<8);
}

static int
_handle_map(struct Entity *p, int x, int y)
{
	int t = flags(tile_at(x,y));
	if (p)
	{
		if (t != -1 && t&4) { _hitspark(p, x, y); }
		if (t != -1 && t&8) { collect(x, y); }
	}
	return t;
}

static int
_update_player(struct Entity * p)
{
	static int const accel = 3<<4;
	static int const vxmax = 3<<8;
	static int const vymax = 8<<8;
	static int const vy0 = 1248;
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
	if (--(p->hit) < 0) { p->hit = 0; }
	if (p->iframes > max_iframes - no_ctrl_time
	    || p->pos.y > ((32 * 16)<<8))
	{
		dx = jbuf = 0;
	}
	if (--(p->iframes) < 0) { p->iframes = 0; }
	if (jbuf && coyote)
	{
		p->vel.y = -vy0;
		p->turnback_time = 0;
		p->frame = 2;
		jbuf = coyote = 0;
	}
	if ((released & CN_BTN_A) && p->vel.y < 0)
	{
		p->vel.y /= 2;
	}
	p->vel.y += grav;
	if (p->vel.y > vymax)
	{
		p->vel.y = vymax;
	}
	p->pos.y += p->vel.y;
	/* check vertical map-collisions */
	q = p->vel.y <= 0 ? 0 : (8<<8);
	f = 0;
	f |= _handle_map(p, p->pos.x, p->pos.y + q);
	s = p->dir < 0 ? -(11<<8) : (10<<8);
	f |= _handle_map(p, p->pos.x + s, p->pos.y + q);
	s = p->dir < 0 ? -(20<<8) : (19<<8);
	f |= _handle_map(p, p->pos.x + s, p->pos.y + q);
	if (f != -1) { hurt |= f&4; }
	/* you can always land on solid (1),
	 * while semisolid (2) requires having previously been
	 * not within it
	 * */
	if (f&1 || (f&2 && p->vel.y > 0
	            && ((p->pos.y - p->vel.y + (8<<8))>>12
	                < ((p->pos.y + (8<<8))>>12))))
	{
		p->pos.y  = (p->pos.y + q)&~0xfff;
		p->pos.y -= p->vel.y <= 0 ? 1 - (17<<8) : 1;
		p->pos.y -= q;
		if (p->vel.y > 0)
		{
			coyote = 6;
			p->state = 1;
		}
		p->vel.y = 0;
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
	p->vel.x += accel * dx / (p->state == 2 ? 2 : 1);
	if (!dx) { p->vel.x *= 9; p->vel.x /= 10; }
	if (abs(p->vel.x) > vxmax)
	{
		p->vel.x = p->vel.x < 0 ? -vxmax : vxmax;
	}
	p->pos.x += p->vel.x;
	/* check nose-side horizontal map collisions */
	q = p->dir < 0 ? -(23<<8) : (22<<8);
	f = 0;
	f |= _handle_map(p, p->pos.x + q, p->pos.y);
	if (f != -1) { hurt |= f&4; }
	if (f&1)
	{
		p->pos.x  = (p->pos.x + q)&~0xfff;
		p->pos.x -= p->dir < 0 ? 1 - (17<<8) : 1;
		p->pos.x -= q;
		p->vel.x = 0;
	}
	/* check interior horizontal map collisions */
	q = p->dir < 0 ? -(9<<8) : (9<<8);
	f = 0;
	f |= _handle_map(p, p->pos.x + q, p->pos.y);
	f |= _handle_map(p, p->pos.x + q, p->pos.y + (7<<8));
	if (f != -1) { hurt |= f&4; }
	if (f&1)
	{
		p->pos.x  = (p->pos.x + q)&~0xfff;
		p->pos.x -= p->dir < 0 ? 1 - (17<<8) : 1;
		p->pos.x -= q;
		p->vel.x = 0;
	}
	/* check butt-side horizontal map collisions */
	q = p->dir < 0 ? (3<<8) : -(4<<8);
	f = 0;
	f |= _handle_map(p, p->pos.x + q, p->pos.y);
	f |= _handle_map(p, p->pos.x + q, p->pos.y + (7<<8));
	/* take another bit to know direction */
	if (f != -1) { hurt |= (f&4)>>1; }
	if (f&1)
	{
		p->pos.x  = (p->pos.x + q)&~0xfff;
		p->pos.x += p->dir < 0 ? -1 : (16<<8);
		p->pos.x -= q;
		p->vel.x = 0;
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
		_hurt_player(p, !!(hurt&4) - !!(hurt&2));
		frozen = 6;
		shake_size = 3;
		shake_time = 12;
	}
	/* if sufficiently slow, stop and set state to idle (0) */
	if (!dx && (abs(p->vel.x) < 1<<5))
	{
		p->vel.x = 0;
		if (p->state == 1) { p->state = 0; }
		p->frame = 0;
	}
	/* void damage */
	if (!p->iframes && p->pos.y > (32 * 16)<<8)
	{
		--(p->hp);
		p->iframes = 2;
		if (p->hp < 0) { p->hp = 0; }
	}
	return 1;
}

static void
_draw_player(struct Entity * p)
{
	int const i = p->state == 2 ? 0 : !!(p->turnback_time);
	int const flip = ((p->dir < 0) ^ i) ? FB_FLIP_HORZ : 0;
	int const stage = p->state == 2
		? (abs(p->vel.y) < 100 ? 1 : (p->vel.y < 0 ? 0 : 2))
		: p->frame;
	int const frame = 32 * 4 * (2 * p->state + i) + 8 * stage;
	if (!(p->iframes & 4))
	{
		fb_spr(&cn_screen, fox, frame, 7, 3,
		       ((p->pos.x - cam.x) >> 8) - 7 * 4,
		       ((p->pos.y - cam.y) >> 8) - 3 * 4, flip);
	}
	if (p->hit)
	{
		fb_spr(&cn_screen, ui, 3, 1, 1,
		       ((p->hpos.x - cam.x) >> 8) - 4,
		       ((p->hpos.y - cam.y) >> 8) - 4, 0);
	}
}

static void
_hurt_player(struct Entity *p, int d)
{
	if (!p) { return; }
	if (p->iframes) { return; }
	--(p->hp);
	p->iframes = max_iframes;
	p->vel.x = d * (-p->dir) * (2<<8);
	p->vel.y = -750;
}
