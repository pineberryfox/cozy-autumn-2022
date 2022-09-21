#include <string.h>
#include "common.h"
#include "console.h"
#include "entity.h"
#include "levels.h"
#include "player.h"

#include "eggboss.h"

static int _update_eggb(struct Entity *);
static void _draw_eggb(struct Entity *);
static void _hurt_eggb(struct Entity *, int);

static int _sx;
static int _sy;

struct Entity
spawn_eggb(int x, int y)
{
	struct Entity e;
	memset((void*)(&e), 0, sizeof(e));
	e.pos.x = e.hb1.pos.x = _sx = x;
	e.pos.y = e.hb1.pos.y = _sy = y;
	e.hb1.radius = 8<<8;
	_sx -= 120<<8;
	_sy += 120<<8;
	clamp_cam(&_sx,&_sy);
	_sy -= 16<<8;
	e.frame_hold = 6;
	e.hp = 3;
	e.update = _update_eggb;
	e.draw = _draw_eggb;
	e.hurt = _hurt_eggb;
	return e;
}

static int
_update_eggb(struct Entity * e)
{
	static int _hatch_cooldown;
	if (!e) { return 0; }
	if (!--(e->iframes)) { e->iframes = 0; }
	if (!--_hatch_cooldown) { _hatch_cooldown = 0; }
	if (e->state)
	{
		screen_locked = 1;
		screen_lock.x = _sx;
		screen_lock.y = _sy;
	}
	switch (e->state)
	{
	case 0:
		if (!hit(&player, e->hb1.pos, e->hb1.radius, 0))
		{
			break;
		}
		++(e->state);
		shake_size = 1;
		shake_time += 12;
		_hatch_cooldown = 180;
		break;
	case 1:
		if (_hatch_cooldown) { break; }
		++(e->state);
		shake_size = 2;
		shake_time += 12;
		_hatch_cooldown = 120;
		break;
	case 2:
		if (_hatch_cooldown) { break; }
		++(e->state);
		shake_size = 3;
		shake_time += 60;
		break;
	default:
		hit(&player, e->hb1.pos, e->hb1.radius, 1);
		break;
	}
	return 1;
}

static void
_draw_eggb(struct Entity * e)
{
	int eye_y = ((e->pos.y - cam.y)>>8)-5;
	if (player.pos.y > e->pos.y + (10<<8)) { ++eye_y; }
	if (player.pos.y < e->pos.y - (10<<8)) { --eye_y; }
	fb_spr(&cn_screen, enemytex, 2 * e->state, 2, 2,
	       ((e->pos.x - cam.x) >> 8) - 8,
	       ((e->pos.y - cam.y) >> 8) - 14, 0);
	fb_spr(&cn_screen, enemytex, 16, 4, 2,
	       ((e->pos.x - cam.x) >> 8) - 16,
	       ((e->pos.y - cam.y) >> 8) - 6, 0);
	if (e->state == 3)
	{
		fb_pixel(&cn_screen,
		         ((e->pos.x - cam.x) >> 8)
		         - (e->pos.x > player.pos.x),
		         eye_y, 19);
	}
}

static void
_hurt_eggb(struct Entity *e, int _d)
{
	(void)(_d) /* unused */;
	if (!e) { return; }
	if (e->iframes) { return; }
	--(e->hp);
	e->iframes = 64;
}
