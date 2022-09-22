#ifndef CAJ_COMMON_H
#define CAJ_COMMON_H
#ifndef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "audio.h"
#include "entity.h"
extern int game_state;
extern char * bgm_data;
/* audio */
extern SDL_AudioSpec audio_format;
extern struct SoundManager sound_manager;
extern int force_bgm_pause;
/* images */
extern SDL_Texture *enemytex;
extern SDL_Texture *fox;
extern SDL_Texture *ui;
/* camera / game updates */
extern struct V2I cam;
extern struct V2I screen_lock;
extern int screen_locked;
extern int shake_size;
extern int shake_time;
extern int frozen;
/* entities */
extern struct Entity enemies[16];
extern unsigned int num_enemies;
/* button state */
extern int pressed;
extern int released;
#endif
