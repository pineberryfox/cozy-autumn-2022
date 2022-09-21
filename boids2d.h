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
	float target_mindist;
	float target_strength;
	float align_awareness;
	float align_strength;
	float cohere_awareness;
	float cohere_strength;
	float uncollide_awareness;
	float uncollide_strength;
	unsigned int num_boids;
};

void init_flock_defaults(struct B2D_Flock *);
void update_flock(struct B2D_Flock *);

#endif
