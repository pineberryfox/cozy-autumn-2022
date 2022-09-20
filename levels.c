#include <stdio.h>
#include "common.h"
#include "console.h"
#include "framebuffer.h"
#include "graphics.h"
#include "levels.h"
#include "player.h"

#define COL_SIZE (MAX_SCREENS_HIGH * MAP_SCREEN_HEIGHT)
#define MAP_SIZE (MAX_MAP_WIDTH * COL_SIZE)
#define _map_length 240

extern unsigned char const lv00[];
static unsigned char const _flags00[] = {
	0, 0, 0, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 5, 0, 1, 1, 0,
	1, 1, 1, 5, 1, 1, 1, 1,
	2, 2, 2, 2, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 2, 2, 2, 2, 2,
	0, 8, 0, 0, 0, 0, 0, 0,
};

static unsigned char const *_flagp;
static unsigned char const *_levelp;
static int _map_height_octets;
static unsigned int _map_column;
/* static unsigned int _collectibles; */
static unsigned int _num_collectibles;
static unsigned int _player_base_x;
static unsigned int _player_base_y;
static unsigned char _map[MAP_SIZE];
static unsigned char const * const _levels[] = {
	lv00,
};
static unsigned char const * const _flags[] = {
	_flags00,
};

SDL_Texture *skybox;
SDL_Texture *tiles;

int
load_level(int n)
{
	char buf[16];
	int i;
	if ((unsigned int)(n) >= sizeof(_levels)/sizeof(_levels[0]))
	{
		return 0;
	}

	snprintf(buf, 16, "skybox%02d.png", n);
	skybox = load_spritesheet(buf);
	snprintf(buf, 16, "tileset%02d.png", n);
	tiles = load_spritesheet(buf);

	_levelp = _levels[n];
	_flagp = _flags[n];
	_map_height_octets = (int)(*(_levelp++));
	_map_column = 0;

	for (i = 0; i <= 16; ++i)
	{
		load_column();
	}
	reset_level();
	return 1;
}
void
reset_level(void)
{
	init_player(&player,
	            (_player_base_x * 16 + 8)<<8,
	            (_player_base_y * 16 + 8)<<8);
}

void
load_column(void)
{
	int i;
	unsigned int m = 0;
	int x;
	if (_map_column >= _map_length) { return; }
	for (i = 0; i < _map_height_octets; ++i)
	{
		m >>= 8;
		m |= (*(_levelp++)) << 24;
	}
	for (i = 0; i < COL_SIZE; ++i)
	{
		x = (m & 1) ? *(_levelp++) : 0;
		if (flags(x) != -1 && (flags(x) & 8))
		{
			++_num_collectibles;
		}
		switch (x)
		{
		case 64:
			_player_base_x = _map_column;
			_player_base_y = i;
			break;
		default:
			break;
		}
		if (x >= 64) { x = 0; }
		_map[COL_SIZE * _map_column + i] = x;
		m >>= 1;
	}
	++_map_column;
}

int
tile_at(int x, int y)
{
	int xc = x>>12;
	int yc = y>>12;
	if (xc < 0 || xc >= _map_length) { return -1; }
	if (yc < 0) { yc = 0; }
	if (yc >= COL_SIZE) { yc = COL_SIZE - 1; }
	return _map[COL_SIZE*xc + yc];
}

int
flags(int t)
{
	if (t < 0 || t > 63) { return -1; }
	return _flagp[t];
}

void
map(int cx, int cy)
{
	int i;
	int j;
	int const r = cy >> 4;
	int const c = cx >> 4;
	int m;
	for (i = 0; i <= 16; ++i)
	{
		if (i + c > _map_length) { continue; }
		for (j = 0; j <= 16; ++j)
		{
			if (j + r > COL_SIZE) { continue; }
			m = _map[COL_SIZE*(i + c) + (j + r)];
			fb_spr(&cn_screen, tiles,
			       32*(m/8) + 2*(m%8), 2, 2,
			       (16 * i) - (cx & 15),
			       (16 * j) - (cy & 15), 0);
		}
	}
}

void
collect(int x, int y)
{
	int const r = y >> 12;
	int const c = x >> 12;
	unsigned int m = COL_SIZE * c + r;
	if (!(flags(_map[m]) & 8)) { return; }
	--_num_collectibles;
	--(_map[COL_SIZE * c + r]);
}

static int
clamp(int mn, int x, int mx)
{
	if (x < mn) { return mn; }
	if (x > mx) { return mx; }
	return x;
}

void
clamp_cam(int *cx, int *cy)
{
	*cx = clamp(0, *cx, _map_length * 16 - 240);
	*cy = clamp(0, *cy, (COL_SIZE * 16 - 160) << 8);
}
