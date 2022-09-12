#ifndef CONSOLE_H
#define CONSOLE_H
#include "framebuffer.h"
enum CN_Button {
	CN_BTN_A = 1,
	CN_BTN_B = 2,
	CN_BTN_START = 4,
	CN_BTN_SELECT = 8,
	CN_BTN_RIGHT = 16,
	CN_BTN_LEFT = 32,
	CN_BTN_DOWN = 64,
	CN_BTN_UP = 128
};
/* buttons: UDLR-+BA */
extern int cn_buttons;
extern struct FrameBuffer cn_screen;
void cn_init(char * title);
void cn_close(void);
void cn_update(void);
_Bool cn_need_physics(void);
int cn_get_fps(void);
#endif
