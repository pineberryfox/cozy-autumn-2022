#include <stdlib.h>
#include <math.h>

#include "boids2d.h"
#define SENSE 15.0f
#define SEPARATION 10.0f

static float const _align_factor     = 1.0f/32.0f;
static float const _coherence_factor = 1.0f/16.0f;
static float const _uncollide_factor = 1.0f/8.0f;

static struct B2D_V2 const zero;

static float
_dir(struct B2D_Boid * b)
{
	if (!b) { return 0; }
	return atan2f(b->vel.y, b->vel.x);
}

static struct B2D_V2
localize(struct B2D_V2 o, float theta, struct B2D_V2 p)
{
	struct B2D_V2 r;
	float const x = p.x - o.x;
	float const y = p.y - o.y;
	/* complex-multiply (x+iy) by (cos(-theta) + i sin(-theta)) */
	/* recall: cos(-theta) = cos(theta); sin(-theta) = -sin(theta) */
	float const c = cosf(theta);
	float const s = -sinf(theta);
	r.x = x*c - y*s;
	r.y = x*s + y*c;
	return r;
}

static int in_range(struct B2D_V2 a)
{
	return (hypotf(a.y, a.x) <= SENSE);
}

static void
align(struct B2D_Flock *f, int b)
{
	struct B2D_V2 c = zero;
	struct B2D_V2 t;
	struct B2D_Boid *x;
	int n = 0;
	int i;
	if (!f) { return; } /* this should never happen */
	x = &(f->boids[b]);
	if (!x) { return; } /* this should never happen */
	for (i = 0; i < f->num_boids; ++i)
	{
		if (i == b) { continue; }
		t = localize(x->pos, _dir(x), f->boids[i].pos);
		if (!in_range(t)) { continue; }
		c.x += (f->boids[i]).vel.x;
		c.y += (f->boids[i]).vel.y;
		++n;
	}
	if (!n) { return; }
	/* average orientation of nearby neighbours */
	c.x /= n;
	c.y /= n;
	x->vel.x += (c.x - x->vel.x) * _align_factor;
	x->vel.y += (c.y - x->vel.y) * _align_factor;
}

static void
cohere(struct B2D_Flock *f, int b)
{
	struct B2D_V2 c = zero;
	struct B2D_V2 t;
	struct B2D_Boid *x;
	int n = 0;
	int i;
	if (!f) { return; } /* this should never happen */
	x = &(f->boids[b]);
	if (!x) { return; } /* this should never happen */
	for (i = 0; i < f->num_boids; ++i)
	{
		if (i == b) { continue; }
		t = localize(x->pos, _dir(x), f->boids[i].pos);
		if (!in_range(t)) { continue; }
		c.x += f->boids[i].pos.x;
		c.y += f->boids[i].pos.y;
		++n;
	}
	/* always add in the global target */
	c.x += f->target.x;
	c.y += f->target.y;
	++n;
	/* average position of nearby neighbours and target */
	c.x /= n;
	c.y /= n;
	x->vel.x += (c.x - x->pos.x) * _coherence_factor;
	x->vel.y += (c.y - x->pos.y) * _coherence_factor;
}

static void
uncollide(struct B2D_Flock *f, int b)
{
	struct B2D_Boid *x;
	struct B2D_V2 c = zero;
	struct B2D_V2 t;
	int i;
	if (!f) { return; }
	x = &(f->boids[b]);
	if (!x) { return; }
	for (i = 0; i < f->num_boids; ++i)
	{
		if (i == b) { continue; }
		t = localize(x->pos, _dir(x), f->boids[i].pos);
		if (!in_range(t)) { continue; }
		if (hypotf(t.y, t.x) < SEPARATION)
		{
			c.x += x->pos.x - f->boids[i].pos.x;
			c.y += x->pos.y - f->boids[i].pos.y;
		}
	}
	x->vel.x += c.x * _uncollide_factor;
	x->vel.y += c.y * _uncollide_factor;
}

static float
_speed_limit(struct B2D_Flock *f, struct B2D_Boid *x)
{
	struct B2D_V2 v;
	float d;
	if (!f || !x) { return 0.0f; }
	v.x = x->pos.x - f->target.x;
	v.y = x->pos.y - f->target.y;
	d = hypotf(v.y, v.x);
	if (d > 48) { return 8.0f; }
	if (d > 24) { return 5.0f; }
	return 3.0f;
}

static void
move_boid(struct B2D_Flock *f, int b)
{
	struct B2D_Boid *x;
	float s;
	float t;
	if (!f) { return; }
	x = &(f->boids[b]);
	if (!x) { return; }
	cohere(f, b);
	uncollide(f, b);
	align(f, b);
	s = hypotf(x->vel.y, x->vel.x);
	t = _speed_limit(f,x);
	if (s > t)
	{
		x->vel.x /= s;
		x->vel.y /= s;
		x->vel.x *= t;
		x->vel.y *= t;
	}
	x->pos.x += x->vel.x;
	x->pos.y += x->vel.y;
}

void
update_flock(struct B2D_Flock *f)
{
	/* specifically do not work on a copy
	 * so that things will separate if they attach
	 */
	int i;
	if (!f) { return; }
	for (i = 0; i < f->num_boids; ++i)
	{
		move_boid(f, i);
	}
}
