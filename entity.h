#ifndef ENTITY_H
#define ENTITY_H
struct V2I
{
	int x;
	int y;
};
struct Hitsphere
{
	struct V2I pos;
	unsigned int radius;
};
struct Entity
{
	int (*update)(struct Entity *);
	void (*draw)(struct Entity *);
	void (*hurt)(struct Entity *, int);
	struct V2I pos;
	struct Hitsphere hb1;
	struct Hitsphere hb2;
	struct V2I vel;
	struct V2I hpos;
	int has_two_spheres;
	int hit;
	int dir;
	int frame;
	int frame_hold;
	int state;
	int turnback_time;
	int iframes;
	int hp;
};
int hit(struct Entity *, struct V2I, unsigned int, int);
#endif
