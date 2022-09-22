#ifndef PTJ_AUDIO_H
#define PTJ_AUDIO_H

#include <pocketmod.h>

extern int bgm_paused;

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
 *                   int: sample rate
 */
void enqueue_sfx(struct SoundManager *, char *, size_t, int);

/* jump_to_pattern
 * struct SoundManager *: the sound system
 *                   int: the pattern
 */
void jump_to_pattern(struct SoundManager *, int);

#endif
