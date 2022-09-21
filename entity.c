#include <math.h>
#include "entity.h"

static void
_hitspark(struct Entity *p, int x, int y)
{
	if (!p) { return; }
	p->hpos.x = x;
	p->hpos.y = y;
	p->hit = 1;
}

static void
_hit_on_circ(struct Entity * p, struct V2I pc, unsigned int pr,
             struct V2I c, void(*hurt)(struct Entity*, int))
{
	int dist;
	long long t;
	int r2 = (int)(((long long)pr * (long long)pr)>>8);
	float theta;
	if (!p) { return; }

	/* determine if c is within the <pc,pr> circle */
	t = (long long)(pc.x - c.x);
	t *= t;
	dist = (int)(t >> 8);
	t = (long long)(pc.y - c.y);
	t *= t;
	dist += (int)(t >> 8);
	if (dist <= r2)
	{
		_hitspark(p, c.x, c.y);
		hurt(p, (c.x >= pc.x) - (c.x < pc.x));
		return;
	}
	/* if not, then p's boundary on the pc-c line is a hit */
	c.x -= pc.x;
	c.y -= pc.y;
	theta = atan2f(c.y, c.x);
	c.x = pc.x + (int)lroundf(pr * cosf(theta));
	c.y = pc.y + (int)lroundf(pr * sinf(theta));
	_hitspark(p, c.x, c.y);
	hurt(p, (c.x >= pc.x) - (c.x < pc.x));
}

static int
_try_hit(struct Entity *p, struct V2I c, unsigned int r, int damaging,
	struct Hitsphere *hb)
{
	struct V2I poffset;
	long long t;
	int dist;
	int r2;
	int pr;

	if (!p || !hb) { return 0; }
	poffset.x = hb->pos.x + ((p->dir > 0)<<8);
	poffset.x *= (p->dir > 0) - (p->dir <= 0);
	poffset.y = hb->pos.y;
	pr = hb->radius;
	r2 = (int)(((long long)(pr+r) * (long long)(pr+r))>>8);
	t = (long long)(p->pos.x + poffset.x - c.x);
	t *= t;
	dist = (int)(t >> 8);
	t = (long long)(p->pos.y + poffset.y - c.y);
	t *= t;
	dist += (int)(t >> 8);
	if (dist <= (long long)(r2))
	{
		if (damaging)
		{
			poffset.x += p->pos.x;
			poffset.y += p->pos.y;
			_hit_on_circ(p, poffset, pr, c, p->hurt);
		}
		return 1;
	}
	return 0;
}

int
hit(struct Entity * p, struct V2I c, unsigned int r, int damaging)
{
	if (!p) { return 0; }
	if (_try_hit(p,c,r,damaging,&(p->hb1))) { return 1; }
	if (!(p->has_two_spheres)) { return 0; }
	return _try_hit(p,c,r,damaging,&(p->hb2));
}
