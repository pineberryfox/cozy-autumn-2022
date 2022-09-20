#include "console.h"
#include "framebuffer.h"

int cn_buttons;
struct FrameBuffer cn_screen;

static _Bool _fullscreen;
static Uint64 _prev_time;
static Uint64 _now;
static double _time_accumulator;
static double const _dt = 1.0 / 60.0;
static double _times[16];
static int _time_i;

static SDL_GameController* _pads[16];
static int _pad_ids[16];
static unsigned int _num_pads;

void (*cn_quit_hook)(void) = NULL;

static void
add_controller(int device_id)
{
	SDL_GameController *pad;
	int id;
	if (SDL_IsGameController(device_id))
	{
		if (_num_pads >= sizeof(_pads) / sizeof(_pads[0]))
		{
			return;
		}
		pad = SDL_GameControllerOpen(device_id);
		if (pad)
		{
			id = SDL_JoystickInstanceID(
				SDL_GameControllerGetJoystick(pad));
			_pads[_num_pads] = pad;
			_pad_ids[_num_pads] = id;
			++_num_pads;
		}
	}
}
static void
remove_controller(int id)
{
	unsigned int i;
	int x = 0;
	for (i = 0; i < sizeof(_pads) / sizeof(_pads[0]); ++i)
	{
		if (_pad_ids[i] == id)
		{
			SDL_GameControllerClose(_pads[i]);
			x = 1;
			break;
		}
	}
	if (!x) { return; }
	_pads[i] = _pads[_num_pads - 1];
	_pad_ids[i] = _pad_ids[_num_pads - 1];
	--_num_pads;
}
static int
button_map(SDL_GameControllerButton b)
{
	switch (b)
	{
	case SDL_CONTROLLER_BUTTON_A:
		return CN_BTN_A;
	case SDL_CONTROLLER_BUTTON_B:
		return CN_BTN_A;
	case SDL_CONTROLLER_BUTTON_X:
		return CN_BTN_B;
	case SDL_CONTROLLER_BUTTON_Y:
		return CN_BTN_B;
	case SDL_CONTROLLER_BUTTON_START:
		return CN_BTN_START;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		return CN_BTN_LEFT;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		return CN_BTN_RIGHT;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		return CN_BTN_UP;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		return CN_BTN_DOWN;
	default:
		break;
	}
	return 0;
}

static int
axis_map(SDL_GameControllerAxis a, int pos)
{
	switch (a)
	{
	case SDL_CONTROLLER_AXIS_LEFTY:
		return pos<0 ? CN_BTN_UP : CN_BTN_DOWN;
	case SDL_CONTROLLER_AXIS_LEFTX:
		return pos<0 ? CN_BTN_LEFT : CN_BTN_RIGHT;
	default:
		break;
	}
	return 0;
}

static void
_quit(int status)
{
#ifndef __EMSCRIPTEN__
	if (cn_quit_hook) { cn_quit_hook(); }
	SDL_DestroyWindow(cn_screen.win);
	SDL_Quit();
	exit(status);
#endif
}

void
cn_init(char *title)
{
	unsigned int i;
	for (i = 0; i < sizeof(_times)/sizeof(_times[0]); ++i)
	{
		_times[i] = 0;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
#ifdef __APPLE__
	SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "1");
#endif
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO
	         | SDL_INIT_GAMECONTROLLER);
	SDL_ShowCursor(SDL_DISABLE);
	if (!title)
	{
		title="GAME";
	}
	fb_init(&cn_screen, 240, 160, title);
	_prev_time = SDL_GetPerformanceCounter();
	_now = _prev_time;
	for (i = 0; i < sizeof(_pads)/sizeof(_pads[0]); ++i)
	{
		/* try to add all the controllers on startup! */
		if (i >= (unsigned int)(SDL_NumJoysticks())) { break; }
		add_controller(i);
	}
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
		case SDL_CONTROLLERDEVICEADDED:
			add_controller(e.cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			remove_controller(e.cdevice.which);
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			cn_buttons |= button_map(e.cbutton.button);
			break;
		case SDL_CONTROLLERBUTTONUP:
			cn_buttons &= ~button_map(e.cbutton.button);
			break;
		case SDL_CONTROLLERAXISMOTION:
			/* dead zone */
			cn_buttons &= ~axis_map(e.caxis.axis, -30000);
			cn_buttons &= ~axis_map(e.caxis.axis,  30000);
			if (500 < e.caxis.value || e.caxis.value < -500)
			{
				cn_buttons |= axis_map(e.caxis.axis,
				                       e.caxis.value);
			}
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
			case SDL_SCANCODE_F10:
				_fullscreen = !_fullscreen;
				SDL_SetWindowFullscreen
					(cn_screen.win,
					 _fullscreen
					 ? SDL_WINDOW_FULLSCREEN_DESKTOP
					 : 0);
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
		default:
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
	unsigned int i;
	for (i = 0; i < sizeof(_times)/sizeof(_times[0]); ++i)
	{
		ftimes += _times[i];
	}
	return (int)((double)(sizeof(_times)/sizeof(_times[0]))
	             / ftimes + 0.5);
}
