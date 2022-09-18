#ifndef BOIDS2D
#define BOIDS2D

struct B2D_V2
{
	float x;
	float y;
};

struct B2D_Boid
{
	struct B2D_V2 pos;
	struct B2D_V2 vel;
	unsigned char data;
};

/* fixed capacity flock */
struct B2D_Flock
{
	struct B2D_Boid boids[16];
	struct B2D_V2 target;
	int num_boids;
};

void update_flock(struct B2D_Flock *);

#endif
