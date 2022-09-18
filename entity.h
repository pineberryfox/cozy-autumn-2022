#ifndef ENTITY_H
#define ENTITY_H
struct Entity
{
	int x;
	int y;
	int vx;
	int vy;
	int dir;
	int frame;
	int frame_hold;
	int state;
	int turnback_time;
	int iframes;
	int hp;
};
#endif
