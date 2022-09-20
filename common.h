#ifndef CAJ_COMMON_H
#define CAJ_COMMON_H
#ifndef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
/* images */
extern SDL_Texture *fox;
/* camera / game updates */
extern int cx;
extern int cy;
extern int shake_time;
extern int frozen;
/* hitspark */
extern int hx;
extern int hy;
/* button state */
extern int pressed;
extern int released;
#endif
