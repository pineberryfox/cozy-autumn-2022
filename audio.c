#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define POCKETMOD_IMPLEMENTATION
#include <pocketmod.h>

#include "audio.h"
#include "common.h"

int bgm_paused;

char *
init_data(char const * fp, size_t * mod_size)
{
	FILE * mod_file = fopen(fp, "rb");
	if (!mod_file) {return NULL;}
	fseek(mod_file, 0L, SEEK_END);
	*mod_size = ftell(mod_file);
	rewind(mod_file);
	char * mod_data = malloc(*mod_size);
	if (!mod_data) {
		fclose(mod_file);
		return NULL;
	}
	size_t i = 0;
	while (i < *mod_size && !ferror(mod_file))
	{
		i += fread(mod_data + i, 1, (*mod_size) - i, mod_file);
	}
	if (ferror(mod_file))
	{
		fclose(mod_file);
		free(mod_data);
		return NULL;
	}
	fclose(mod_file);
	return mod_data;
}

char *
init_context(struct pocketmod_context * context, char const * fp, int rate)
{
	size_t mod_size;
	char * mod_data = init_data(fp, &mod_size);
	if (!mod_data) {return NULL;}
	if (!pocketmod_init(context, mod_data, mod_size, rate))
	{
		free(mod_data);
		return NULL;
	}
	return mod_data;
}

void
mix(struct SoundManager * manager,
    float * restrict buffer, float * restrict * scratch,
    unsigned int bytes)
{
	unsigned int i = 0;
	if (bgm_paused)
	{
		if (buffer) { memset((void*)(buffer), 0, bytes); }
	}
	if (!scratch || !buffer || !manager || !manager->bgm) {return;}
	if (!*scratch) {*scratch = malloc(bytes);}
	if (!*scratch) {return;}
	while (i < bytes && !bgm_paused)
	{
		i += pocketmod_render(manager->bgm,
		                      (char*)(buffer) + i, bytes - i);
	}
	struct sfx_list * p = NULL;
	struct sfx_list * s = manager->sfx;
	while (s)
	{
		i = pocketmod_render(s->data, (char*)(* scratch), bytes);
		for (;i < bytes; ++i)
		{
			((char*)(*scratch))[i] = 0;
		}
		for (i = 0; i < bytes / sizeof(float); ++i)
		{
			buffer[i] += (*scratch)[i];
		}
		if (!pocketmod_loop_count(s->data))
		{
			p = s;
			s = s->next;
			continue;
		}
		if (p)
		{
			p->next = s->next;
			free(s->data);
			free(s);
			s = p->next;
			continue;
		}
		manager->sfx = s->next;
		free(s->data);
		free(s);
		s = manager->sfx;
	}
}

void
enqueue_sfx(struct SoundManager * manager,
            char * data, size_t size, int n)
{
	struct sfx_list * s = malloc(sizeof(struct sfx_list));
	if (!s) {return;}
	s->data = malloc(sizeof(struct pocketmod_context));
	if (!s->data)
	{
		free(s);
		return;
	}
	if (!pocketmod_init(s->data, data, size, audio_format.freq))
	{
		free(s);
		return;
	}
	jump_to_pattern(s->data, n);
	/* acquire a lock to avoid mix invalidating our data */
	SDL_LockAudioDevice(audio_device);
	s->next = manager->sfx;
	manager->sfx = s;
	SDL_UnlockAudioDevice(audio_device);
}

/* this function jumps into the private parts of pocketmod! */
void
jump_to_pattern(struct pocketmod_context *ctx, int n)
{
	if (!ctx) { return; }
	if (n >= ctx->num_patterns ) { return; }
	ctx->pattern = n;
	ctx->line = -1;
	ctx->tick = ctx->ticks_per_line - 1;
	ctx->loop_count = 0;

}
