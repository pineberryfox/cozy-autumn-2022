#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <pocketmod.h>
#include "audio.h"
#include "boids2d.h"
#include "common.h"
#include "console.h"
#include "graphics.h"
#include "levels.h"
#include "player.h"

/* constants */
static int const _shakex[] = {0, 2, 0, -1, 1, -2};
static int const _shakey[] = {1, 2, 0, -2, -1, 0};
#define _shakes (sizeof(_shakex)/sizeof(_shakex[0]))

/* exported */
struct SoundManager sound_manager;
char *bgm_data;
char *sfx_data;
size_t sfx_size;
struct Entity enemies[16];
struct V2I cam;
struct V2I screen_lock;
unsigned int num_enemies;
SDL_AudioDeviceID audio_device;
SDL_AudioSpec audio_format;
SDL_Texture *enemytex;
SDL_Texture *fox;
SDL_Texture *ui;
int force_bgm_pause;
int frozen;
int game_state;
int screen_locked;
int pressed;
int released;
int shake_size;
int shake_time;

/* nonexported */
static struct FrameBuffer *fb;
static struct B2D_Flock hp_flock;
static struct pocketmod_context * _pmcontext;
static SDL_Texture *logo;
static SDL_Texture *title;
static float * _scratch;
static int _level;
static int ncx;
static struct V2I rcam;
static int cyt;
static int _paused;
static int _should_reset;

static void
cleanup(void)
{
	SDL_PauseAudioDevice(audio_device, 1);
	if (_pmcontext) { free(_pmcontext); _pmcontext = NULL; }
	if (_scratch) { free(_scratch); _scratch = NULL; }
	if (bgm_data) { free(bgm_data); bgm_data = NULL; }
	if (sfx_data) { free(sfx_data); sfx_data = NULL; }
	if (fox) { SDL_DestroyTexture(fox); fox = NULL; }
	if (skybox) { SDL_DestroyTexture(skybox); skybox = NULL; }
	if (tiles) { SDL_DestroyTexture(tiles); tiles = NULL; }
}

static void
_audio_callback(void * userdata, unsigned char * buffer, int bytes)
{
	mix(userdata, (float*)(buffer), &_scratch, bytes);
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
	unsigned int i;
	float d;
	init_flock_defaults(&hp_flock);
	hp_flock.num_boids = (player.hp - 1) / 3;
	hp_flock.target.x = player.pos.x / 256.0f;
	hp_flock.target.y = (player.pos.y - (16<<8)) / 256.0f;
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
	unsigned int i;
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
		             (int)(lroundf(p.x-v.x-cam.x/256.0f) - 1),
		             (int)(lroundf(p.y-v.y-cam.y/256.0f) - 1),
		             3,3,
		             (0 <= d && d < 2) ? _colours[d] : 0);
		fb_fill_rect(&cn_screen,
		             (int)(lroundf(p.x - cam.x/256.0f) - 1),
		             (int)(lroundf(p.y - cam.y/256.0f) - 1),
		             3,3,
		             (0 <= d && d < 2) ? _colours[d] : 0);
		fb_fill_rect(&cn_screen,
		             (int)(lroundf(p.x + v.x - cam.x/256.0f)),
		             (int)(lroundf(p.y + v.y - cam.y/256.0f)),
		             3,3,
		             19);
		fb_pixel(&cn_screen,
		         (int)(lroundf(p.x + 2*v.x - cam.x/256.0f) + 1),
		         (int)(lroundf(p.y + 2*v.y - cam.y/256.0f)),
		         0);
	}
}

static void
force_player_into_screen(void)
{
	int ldx = player.dir > 0 ? 4<<8 : 23<<8;
	int rdx = player.dir > 0 ? 23<<8 : 4<<8;
	if (player.pos.x < cam.x + ldx)
	{
		player.pos.x = cam.x + ldx;
		if (player.vel.x < 0) { player.vel.x = 0; }
	}
	if (player.pos.x > cam.x + (240<<8) - rdx)
	{
		player.pos.x = cam.x + (240<<8) - rdx;
		if (player.vel.x > 0) { player.vel.x = 0; }
	}
}

static void
adjust_cam(void)
{
	static int const _cdist = 4;
	static int const _ytrack = 3<<7;
	if (screen_locked)
	{
		rcam.x += ((rcam.x>>8 < screen_lock.x>>8)<<8)
			- ((rcam.x>>8 > screen_lock.x>>8)<<8);
		rcam.y += ((rcam.y>>8 < screen_lock.y>>8)<<8)
			- ((rcam.y>>8 > screen_lock.y>>8)<<8);
		return;
	}
	if (((player.pos.x - rcam.x) >> 8) > 120 + _cdist)
	{
		rcam.x = player.pos.x - ((120 + _cdist)<<8);
	}
	if (((player.pos.x - rcam.x) >> 8) < 120 - _cdist)
	{
		rcam.x = player.pos.x - ((120 - _cdist)<<8);
	}
	if (player.state != 2)
	{
		cyt = player.pos.y - (54<<8);
	}
	if (player.vel.y > 0 && ((player.pos.y - rcam.y)>>8) > 96)
	{
		rcam.y = player.pos.y - (96 << 8);
	}
	if (abs(rcam.y-cyt) < _ytrack)
	{
		rcam.y = cyt;
	}
	else
	{
		rcam.y += _ytrack * (!!(rcam.y < cyt) - !!(rcam.y > cyt));
	}
}

static void
main_update(void)
{
	unsigned int i;
	hp_flock.target.x = player.pos.x
		+ (player.dir > 0 ? -(16<<8) : 16<<8);
	hp_flock.target.x /= 256.0f;
	hp_flock.target.y = player.pos.y - (16<<8);
	hp_flock.target.y /= 256.0f;
	/* physics frames */
	while (cn_need_physics())
	{
		if (pressed & CN_BTN_START)
		{
			pressed = 0;
			released = 0;
			_paused = !_paused;
			sfx(SFX_SELECT);
			bgm_paused = _paused && !force_bgm_pause;
		}
		if (_paused) { continue; }
		if (--shake_time < 0) { shake_time = 0; }
		if (--frozen < 0) { frozen = 0; }
		if (!frozen)
		{
			if (_should_reset)
			{
				_should_reset = 0;
				ncx = reset_level();
				clamp_cam(&cam.x, &cam.y);
				rcam = cam;
				init_flock();
			}
			screen_locked = 0;
			force_bgm_pause = 0;
			for (i = 0; i < num_enemies; ++i)
			{
				if (!(enemies[i].update(&(enemies[i]))))
				{
					--num_enemies;
					enemies[i] = enemies[num_enemies];
					--i;
				}
			}
			bgm_paused = force_bgm_pause;
			player.update(&player);
			if (screen_locked)
			{
				force_player_into_screen();
			}
			update_flock(&hp_flock);
			if (!player.hp)
			{
				frozen = 60;
				player.iframes = 0;
				_should_reset = 1;
			}
		}
		pressed = 0;
		released = 0;
		adjust_cam();
		clamp_cam(&rcam.x, &rcam.y);
	}
	/* actual frames */
	while (rcam.x > (ncx<<8))
	{
		load_column();
		ncx += 16;
	}
}

static void
main_draw(void)
{
	unsigned int i;
	int t;
	fb_spr(&cn_screen, skybox, 0, 32, 32, -8, -48, 0);
	cam.x = rcam.x + ((shake_time
	                   ? shake_size
	                   * _shakex[(shake_time-1) % _shakes]
	                   : 0) << 8);
	cam.y = rcam.y + ((shake_time
	                   ? shake_size
	                   * _shakey[(shake_time-1) % _shakes]
	                   : 0) << 8);
	clamp_cam(&(cam.x), &(cam.y));
	map(cam.x>>8, cam.y>>8);
	for (i = 0; i < num_enemies; ++i)
	{
		enemies[i].draw(&(enemies[i]));
	}
	player.draw(&player);

	t = player.hp - 1;
	i = 0;
	while (t > 0 && i < 16)
	{
		hp_flock.boids[i].data = t - 1 > 1 ? 1 : t - 1;
		if (i >= hp_flock.num_boids)
		{
			hp_flock.boids[i].pos.x
				= player.pos.x / 256.0f;
			hp_flock.boids[i].pos.y
				= player.pos.y / 256.0f;
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
#ifdef DEBUG
	{
		static char buf[16];
		i = snprintf(buf, 16, "%d", cn_get_fps());
		fb_text(&cn_screen, buf, 240 - 8 * i, 0, 19);
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
}

static void
step(void)
{
	static int _logo_time = 120;
	int i = cn_buttons;
	cn_update();
	pressed  |= (cn_buttons ^ i) &  cn_buttons;
	released |= (cn_buttons ^ i) & ~cn_buttons;

	SDL_SetRenderDrawColor(fb->_renderer, 0, 0, 0, 255);
	fb_fill(fb, -1);
	switch (game_state)
	{
	case 0:
		bgm_paused = 1;
		while (cn_need_physics()) { --_logo_time; }
		SDL_SetRenderDrawColor(fb->_renderer, 0, 47, 88, 255);
		fb_fill(fb, -1);
		SDL_SetRenderDrawColor(fb->_renderer, 243, 111, 188, 255);
		fb_spr(&cn_screen, logo, 0, 8, 8, 88, 40, 0);
		fb_text(&cn_screen, "PINEBERRY", 80, 88, -1);
		fb_text(&cn_screen, "FOX", 104, 96, -1);
		if (_logo_time <= 0)
		{
			SDL_DestroyTexture(logo);
			logo = NULL;
			++game_state;
		}
		pressed = 0;
		released = 0;
		break;
	case 1:
		/* title screen */
		bgm_paused = 1;
		while (cn_need_physics());
		if (pressed & (CN_BTN_A | CN_BTN_B | CN_BTN_START))
		{
			pressed = released = 0;
			_level = -1;
			game_state = 3;
			sfx(SFX_SELECT);
		}
		fb_spr(&cn_screen, title, 0, 30, 20, 0, 0, 0);
		fb_text(&cn_screen, "PRESS START", 76, 128, 0);
		break;
	case 2:
		main_update();
		main_draw();
		break;
	case 3:
		bgm_paused = 1;
		while (cn_need_physics());
		++_level;
		game_state += load_level(_level) ? -1 : 1;
		rcam = cam;
		ncx = 0;
		init_flock();
		cyt = cam.y;
		break;
	case 4:
		/* end screen */
		bgm_paused = 1;
		while (cn_need_physics());
		if (pressed & (CN_BTN_A | CN_BTN_B | CN_BTN_START))
		{
			pressed = released = 0;
			game_state = 1;
		}
		fb_fill(&cn_screen, 19);
		fb_text(&cn_screen, "THANK YOU", 84, 60, 0);
		fb_text(&cn_screen, "FOR PLAYING", 76, 68, 0);
		fb_text(&cn_screen, "THIS DEMO", 84, 76, 0);
		fb_text(&cn_screen, "ENJOY THE EGGS!", 60, 92, 0);
		break;
	default:
		bgm_paused = 1;
		break;
	}
	fb_show(&cn_screen);
#ifdef __EMSCRIPTEN__
	EM_ASM({Module['fps_counter'].innerText = $0;}, cn_get_fps());
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

	_pmcontext = malloc(sizeof(struct pocketmod_context));
	if (!_pmcontext)
	{
		fprintf(stderr, "no memory for context\n");
		return 1;
	}
	memset(_pmcontext, 0, sizeof(*_pmcontext));
	sound_manager.bgm = _pmcontext;
	sound_manager.sfx = NULL;
	audio_format.freq = 44100;
	audio_format.format = AUDIO_F32;
	audio_format.channels = 2;
	audio_format.samples = 2048;
	audio_format.callback = _audio_callback;
	audio_format.userdata = (void*)(&sound_manager);
	audio_device
		= SDL_OpenAudioDevice(NULL, 0,
		                      &audio_format, &audio_format,
		                      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (!audio_device)
	{
		fprintf(stderr,
		        "error: SDL_OpenAudioDevice() failed: %s\n",
		        SDL_GetError());
		return 1;
	}
	bgm_paused = 1;
	SDL_PauseAudioDevice(audio_device, 0);

	sfx_data = init_data("sfx.mod", &sfx_size);
	fox = load_spritesheet("fox.png");
	logo = load_spritesheet("logo.png");
	title = load_spritesheet("title.png");
	ui = load_spritesheet("ui.png");

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(step, 0, 1);
#else
	while (1) { step(); }
#endif
	return 0;
}
