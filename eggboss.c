#include <string.h>
#include "audio.h"
#include "common.h"
#include "console.h"
#include "entity.h"
#include "levels.h"
#include "player.h"

#include "eggboss.h"

static struct Entity _spawn_tegg(int x, int y);
static int _update_tegg(struct Entity *);
static void _draw_tegg(struct Entity *);
static void _hurt_tegg(struct Entity *, int);
static int _update_eggb(struct Entity *);
static void _draw_eggb(struct Entity *);
static void _hurt_eggb(struct Entity *, int);

static int _sx;
static int _sy;

static struct V2I
_add(struct V2I a, struct V2I b)
{
	struct V2I r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	return r;
}

struct Entity
spawn_eggb(int x, int y)
{
	struct Entity e;
	memset((void*)(&e), 0, sizeof(e));
	e.pos.x = _sx = x;
	e.pos.y = _sy = y;
	e.hb1.radius = 8<<8;
	_sx -= 120<<8;
	_sy += 120<<8;
	clamp_cam(&_sx,&_sy);
	_sy -= 16<<8;
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
	if (--(e->iframes) < 0) { e->iframes = 0; }
	if (--(e->hit) < 0) { e->hit = 0; }
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
		if (!hit(&player, _add(e->pos, e->hb1.pos),
		         e->hb1.radius, 0))
		{
			break;
		}
		force_bgm_pause = 1;
		jump_to_pattern(&sound_manager, 5);
		++(e->state);
		shake_size = 1;
		shake_time += 12;
		_hatch_cooldown = 180;
		break;
	case 1:
		force_bgm_pause = 1;
		if (_hatch_cooldown) { break; }
		++(e->state);
		shake_size = 2;
		shake_time += 12;
		_hatch_cooldown = 120;
		break;
	case 2:
		force_bgm_pause = 1;
		if (_hatch_cooldown) { break; }
		++(e->state);
		shake_size = 3;
		shake_time += 60;
		_hatch_cooldown = 120;
		break;
	case 3:
		if (!player.iframes
		    && hit(&player, _add(e->pos, e->hb1.pos),
		           e->hb1.radius, 1))
		{
			shake_size = 3;
			shake_time = 12;
		}
		if (!_hatch_cooldown)
		{
			_hatch_cooldown = 60;
			int dx = player.pos.x - e->pos.x;
			int dy = player.pos.y - e->pos.y;
			float theta = atan2f(dy, dx);
			if (dy < 0) { theta = (theta - M_PI_2) / 2.0f; }
			int x = (int)(e->pos.x+(12<<8)*cosf(theta));
			int y = (int)(e->pos.y+(12<<8)*sinf(theta));
			struct Entity tegg = _spawn_tegg(x,y);
			tegg.vel.x = (int)((3<<8) * cosf(theta));
			tegg.vel.y = (int)((3<<8) * sinf(theta));
			if (dy < 0)
			{
				tegg.vel.x *= 4.0f/3.0f;
				tegg.vel.y *= 11.0f/10.0f;
			}
			enemies[num_enemies] = tegg;
			++num_enemies;
		}
		if (!e->hp) { ++(e->state); _hatch_cooldown = 120; }
		break;
	case 4:
		if (--_hatch_cooldown > 0) { break; }
		++game_state;
		break;
	default:
		break;
	}
	return 1;
}

static void
_draw_eggb(struct Entity * e)
{
	int eye_y = ((e->pos.y - cam.y)>>8)-5;
	int f;
	if (!e) { return; }
	if (player.pos.y > e->pos.y + (10<<8)) { ++eye_y; }
	if (player.pos.y < e->pos.y - (10<<8)) { --eye_y; }
	if (!(e->iframes & 4))
	{
		f = e->state == 4 ? 32 : 2 * e->state;
		fb_spr(&cn_screen, enemytex, f, 2, 2,
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
	if (e->hit)
	{
		fb_spr(&cn_screen, ui, 3, 1, 1,
		       ((e->hpos.x - cam.x) >> 8) - 4,
		       ((e->hpos.y - cam.y) >> 8) - 4, 0);
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

static struct Entity
_spawn_tegg(int x, int y)
{
	struct Entity e;
	memset((void*)(&e), 0, sizeof(e));
	e.pos.x = x;
	e.pos.y = y;
	e.hb1.radius = 4<<8;
	e.hp = 1;
	e.frame_hold = 60;
	e.update = _update_tegg;
	e.draw = _draw_tegg;
	e.hurt = _hurt_tegg;
	return e;
}

static int
_update_tegg(struct Entity * self)
{
	static int const vymax = 3<<8;
	unsigned int i;
	int f;
	int q;
	if (!self) { return 0; }

	self->vel.y += grav;
	if (self->vel.y > vymax) { self->vel.y = vymax; }
	self->pos.y += self->vel.y;
	/* check vertical map-collisions */
	q = self->vel.y <= 0 ? -(4<<8) : (2<<8);
	f = flags(tile_at(self->pos.x, self->pos.y + q));
	if (f&1 || (f&2 && self->vel.y > 0
	            && ((self->pos.y - self->vel.y + (2<<8))>>12
	                < ((self->pos.y + (2<<8))>>12))))
	{
		self->pos.y  = (self->pos.y + q)&~0xfff;
		self->pos.y -= self->vel.y <= 0 ? 1 - (17<<8) : 1;
		self->pos.y -= q;
		self->vel.y = 0;
		self->vel.x = 0;
		self->hp = 0;
	}

	self->pos.x += self->vel.x;
	/* check horizontal map collisions */
	q = self->vel.x < 0 ? -(4<<8) : (4<<8);
	f = flags(tile_at(self->pos.x + q, self->pos.y));
	if (f&1)
	{
		self->pos.x  = (self->pos.x + q)&~0xfff;
		self->pos.x -= self->dir < 0 ? 1 - (17<<8) : 1;
		self->pos.x -= q;
		self->vel.x = 0;
		self->hp = 0;
	}

	if (!self->hp) { return !!--(self->frame_hold); }
	/* check other collisions */
	for (i = 0; i < num_enemies; ++i)
	{
		if (&(enemies[i]) == self) { continue; }
		if (hit(&(enemies[i]), _add(self->pos, self->hb1.pos),
		        self->hb1.radius, 1))
		{
			self->hurt(self, 0);
			return 1;
		}
	}
	q = player.iframes;
	if (hit(&player, _add(self->pos, self->hb1.pos),
	        self->hb1.radius, 1))
	{
		self->hurt(self, 0);
		if (!q) { shake_size = 3; shake_time = 12; }
	}
	return 1;
}

static void
_draw_tegg(struct Entity * self)
{
	int frame = 1;
	if (!self) { return; }
	if (self->vel.y > 2*self->vel.x) { frame = 0; }
	if (self->vel.x > 2*self->vel.y) { frame = 2; }
	if (self->vel.y > 0) { frame += 8; }
	if (self->hp == 0) { frame = 3 + 8 * (self->frame_hold < 54); }
	frame += 20;
	fb_spr(&cn_screen, enemytex, frame, 1, 1,
	       ((self->pos.x - cam.x)>>8) - 4,
	       ((self->pos.y - cam.y)>>8) - 4,
	       (self->vel.x < 0) ? FB_FLIP_HORZ : 0);
}

static void
_hurt_tegg(struct Entity * self, int _d)
{
	(void)(_d);
	if (!self) { return; }
	if (!--(self->hp)) { self->hp = 0; }
}
