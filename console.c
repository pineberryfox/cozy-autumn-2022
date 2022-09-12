#include "console.h"
#include "framebuffer.h"

int cn_buttons = 0;
struct FrameBuffer cn_screen;

static _Bool _fullscreen = 0;
static Uint64 _prev_time = 0;
static Uint64 _now = 0;
static double _time_accumulator = 0;
static double const _dt = 1.0 / 60.0;
static double _times[16];
static int _time_i = 0;

static void
_quit(int status)
{
#ifndef __EMSCRIPTEN__
	SDL_DestroyWindow(cn_screen.win);
	SDL_Quit();
	exit(status);
#endif
}

void
cn_init(char *title)
{
	int i;
	for (i = 0; i < sizeof(_times)/sizeof(_times[0]); ++i)
	{
		_times[i] = 0;
	}
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
#ifdef __APPLE__
	SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, 0);
#endif
	if (!title)
	{
		title="GAME";
	}
	fb_init(&cn_screen, 240, 160, title);
	_prev_time = SDL_GetPerformanceCounter();
	_now = _prev_time;
	cn_update();
}

void
cn_update(void)
{
	SDL_Event e;
	
	double _frame_time;
	_now = SDL_GetPerformanceCounter();
	_frame_time = (double)(_now - _prev_time)
		/ SDL_GetPerformanceFrequency();
	_times[(_time_i++) % (sizeof(_times)/sizeof(_times[0]))]
		= _frame_time;
	_prev_time = _now;
	_time_accumulator += _frame_time;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_QUIT:
			_quit(0);
			break;
		case SDL_KEYDOWN:
			switch (e.key.keysym.scancode)
			{
			case SDL_SCANCODE_ESCAPE:
				_quit(0);
				break;
			case SDL_SCANCODE_SPACE:
				cn_buttons |= CN_BTN_A;
				break;
			case SDL_SCANCODE_RETURN:
				cn_buttons |= CN_BTN_START;
				break;
			case SDL_SCANCODE_A:
			case SDL_SCANCODE_LEFT:
				cn_buttons |= CN_BTN_LEFT;
				break;
			case SDL_SCANCODE_D:
			case SDL_SCANCODE_RIGHT:
				cn_buttons |= CN_BTN_RIGHT;
				break;
			case SDL_SCANCODE_W:
			case SDL_SCANCODE_UP:
				cn_buttons |= CN_BTN_UP;
				break;
			case SDL_SCANCODE_S:
			case SDL_SCANCODE_DOWN:
				cn_buttons |= CN_BTN_DOWN;
				break;
			default:
				break;
			}
			break;
		case SDL_KEYUP:
			switch (e.key.keysym.scancode)
			{
			case SDL_SCANCODE_SPACE:
				cn_buttons &= ~CN_BTN_A;
				break;
			case SDL_SCANCODE_RETURN:
				cn_buttons &= ~CN_BTN_START;
				break;
			case SDL_SCANCODE_A:
			case SDL_SCANCODE_LEFT:
				cn_buttons &= ~CN_BTN_LEFT;
				break;
			case SDL_SCANCODE_D:
			case SDL_SCANCODE_RIGHT:
				cn_buttons &= ~CN_BTN_RIGHT;
				break;
			case SDL_SCANCODE_W:
			case SDL_SCANCODE_UP:
				cn_buttons &= ~CN_BTN_UP;
				break;
			case SDL_SCANCODE_S:
			case SDL_SCANCODE_DOWN:
				cn_buttons &= ~CN_BTN_DOWN;
				break;
			default:
				break;
			}
			break;
		}
	}
}

_Bool
cn_need_physics(void)
{
	if (_time_accumulator <= _dt) { return 0; };
	_time_accumulator -= _dt;
	return 1;
}

int
cn_get_fps(void)
{
	double ftimes = 0;
	int i;
	for (i = 0; i < sizeof(_times)/sizeof(_times[0]); ++i)
	{
		ftimes += _times[i];
	}
	return (int)((double)(sizeof(_times)/sizeof(_times[0]))
	             / ftimes + 0.5);
}
