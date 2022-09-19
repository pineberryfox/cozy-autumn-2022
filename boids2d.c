#include <stdlib.h>
#include <math.h>

#include "boids2d.h"
#define SENSE 15.0f
#define SEPARATION 10.0f

static struct B2D_V2 const zero;

void
init_flock_defaults(struct B2D_Flock *f)
{
	if (!f) { return; }
	f->target_mindist = 32.0f;
	f->target_strength = 7.0f/8.0f;
	f->align_awareness = 15.0f;
	f->align_strength = 1.0f/16.0f;
	f->cohere_awareness = 75.0f;
	f->cohere_strength = 1.0f/64.0f;
	f->uncollide_awareness = 10.0f;
	f->uncollide_strength = 0.875f;
}

static struct B2D_V2
_add(struct B2D_V2 a, struct B2D_V2 b)
{
	struct B2D_V2 r = a;
	r.x += b.x;
	r.y += b.y;
	return r;
}

static struct B2D_V2
_sub(struct B2D_V2 a, struct B2D_V2 b)
{
	struct B2D_V2 r = a;
	r.x -= b.x;
	r.y -= b.y;
	return r;
}

static struct B2D_V2
_smul(struct B2D_V2 a, float s)
{
	struct B2D_V2 r = a;
	r.x *= s;
	r.y *= s;
	return r;
}

static struct B2D_V2
_sdiv(struct B2D_V2 a, float s)
{
	struct B2D_V2 r = a;
	r.x /= s;
	r.y /= s;
	return r;
}

static struct B2D_V2
_normalize_scale(struct B2D_V2 a, float s)
{
	float h = hypotf(a.y, a.x);
	if (!h) { return a; }
	if (h < 1) { return _smul(a, s); }
	return _sdiv(_smul(a, s), h);
}

static int
in_range(struct B2D_V2 a, float awareness)
{
	return (hypotf(a.y, a.x) <= awareness);
}

static float
_speed_limit(struct B2D_Flock *f, struct B2D_Boid *x)
{
	struct B2D_V2 v;
	float d;
	if (!f || !x) { return 0.0f; }
	v = _sub(f->target, x->pos);
	d = hypotf(v.y, v.x);
	if (d > 48) { return 8.0f; }
	if (d > 24) { return 5.0f; }
	return 3.0f;
}

static void
move_boid(struct B2D_Flock *f, int b)
{
	struct B2D_Boid *x;
	struct B2D_V2 t;
	struct B2D_V2 u;
	struct B2D_V2 valign = zero;
	struct B2D_V2 vcohere = zero;
	struct B2D_V2 vuncollide = zero;
	int balign = 0;
	int bcohere = 0;
	int buncollide = 0;
	float s;
	float limit;
	int i;
	if (!f) { return; }
	x = &(f->boids[b]);
	if (!x) { return; }
	for (i = 0; i < f->num_boids; ++i)
	{
		if (i == b) { continue; }
		t = _sub(f->boids[i].pos, x->pos);
		s = hypotf(t.y, t.x);
		if (s < f->align_awareness)
		{
			++balign;
			valign = _add(valign, f->boids[i].vel);
		}
		if (s < f->cohere_awareness)
		{
			++bcohere;
			vcohere = _add(vcohere, t);
		}
		if (s < f->uncollide_awareness)
		{
			++buncollide;
			t = _sub(x->pos, f->boids[i].pos);
			u = _sdiv(t,
			          powf(1.0f - s/(f->uncollide_awareness),
			               2.0f));
			vuncollide = _add(vuncollide, u);
		}
	}
	/* follow */
	t = _sub(f->target, x->pos);
	if (!in_range(t, f->target_mindist))
	{
		t = _normalize_scale(_sub(t, x->vel),
		                     in_range(t,  2 * f->target_mindist)
		                     ? f->target_strength
		                     : f->target_strength);
		x->vel = _add(x->vel, t);
	}
	/* cohere */
	if (bcohere)
	{
		vcohere = _sdiv(vcohere, bcohere);
		vcohere = _sub(vcohere, x->pos);
		t = _normalize_scale(_sub(vcohere, x->vel),
		                     f->cohere_strength);
		x->vel = _add(x->vel, t);
	}
	/* align */
	if (balign)
	{
		valign = _sdiv(valign, balign);
		t = _normalize_scale(_sub(valign, x->vel),
		                     f->align_strength);
		x->vel = _add(x->vel, t);
	}
	/* uncollide */
	if (buncollide)
	{
		vuncollide = _sdiv(vuncollide, buncollide);
		t = _normalize_scale(_sub(vuncollide, x->vel),
		                     f->uncollide_strength);
		x->vel = _add(x->vel, t);
	}

	s = hypotf(x->vel.y, x->vel.x);
	limit = _speed_limit(f,x);
	if (s > limit)
	{
		x->vel = _sdiv(x->vel, s);
		x->vel = _smul(x->vel, limit);
	}
	x->pos = _add(x->pos, x->vel);
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
