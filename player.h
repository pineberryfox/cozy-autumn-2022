#ifndef PLAYER_H
#define PLAYER_H
#include "entity.h"
extern struct Entity player;

void init_player(struct Entity *, int, int);
void update_player(struct Entity *);
void draw_player(struct Entity * p);

#endif
