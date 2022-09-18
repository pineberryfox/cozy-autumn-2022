#ifndef PLAYER_H
#define PLAYER_H
#include "entity.h"
struct Entity player;

void init_player(struct Entity *);
void update_player(struct Entity *);
void draw_player(struct Entity * p);

#endif
