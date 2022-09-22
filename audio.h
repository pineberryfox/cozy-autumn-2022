#ifndef PTJ_AUDIO_H
#define PTJ_AUDIO_H

#include <pocketmod.h>

extern int bgm_paused;

enum SFX
{
	SFX_RUMBLE = 0,
	SFX_JUMP = 1,
	SFX_COLLECT = 2,
	SFX_HURT = 3,
	SFX_SQUEAK = 4,
	SFX_SELECT = 5,
};

struct sfx_list {
	struct sfx_list * next;
	struct pocketmod_context * data;
};

struct SoundManager {
	struct pocketmod_context * bgm;
	struct sfx_list * sfx;
};

/* init_data
 * char const *: filepath of mod file
 *     size_t *: (output) size of data
 */
char * init_data(char const *, size_t *);

/* init_context
 * struct pocketmod_context *: the context
 *               char const *: filepath of mod file
 *                        int: sample rate
 */
char * init_context(struct pocketmod_context *, char const *, int);
/* use pocketmod_init_context when data is already available */

/* mix
 * struct SoundManager *: data to be mixed
 *               float *: buffer
 *              float **: scratch space
 *          unsigned int: bytes
 * the buffer must be the indicated length,
 * and the scratch space must meet or exceed that length
 */
void mix(struct SoundManager *, float * restrict, float * restrict *,
         unsigned int);
/* call as mix(manager, buffer, &scratch, size) */

/* enqueue_sfx
 * struct SoundManager *: the sound system
 *                char *: MOD file data
 *                size_t: MOD file size
 *                   int: the desired sfx
 */
void enqueue_sfx(struct SoundManager *, char *, size_t, int);
/* sfx: enqueue_sfx with a default parameters */
#define sfx(n) (enqueue_sfx(&sound_manager, sfx_data, sfx_size, n))

/* jump_to_pattern
 * struct pocketmod_context *: the module
 *                        int: the pattern
 */
void jump_to_pattern(struct pocketmod_context *, int);

#endif
