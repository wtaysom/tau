/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <style.h>
#include <mylib.h>
#include <eprintf.h>

typedef struct counter_s	counter_s;
struct counter_s {
	counter_s	*c_next;
	counter_s	*c_head;
	unsigned	c_key;
	int		c_count;
};

typedef struct hash_counter_s {
	unsigned	hc_ticks;
	unsigned	hc_total;
	unsigned	hc_num_buckets;
	counter_s	*hc_clock;
	counter_s	hc_buckets[0];
} hash_counter_s;

#define HASH(_hc, _x)	((_x) % ((_hc)->hc_num_buckets))

void *init_hcounter (unsigned num_buckets, unsigned num_counters)
{
	hash_counter_s	*hc;

	if (!num_counters) {
		num_counters = num_buckets;
	}
	hc = ezalloc(sizeof(hash_counter_s) + num_buckets * sizeof(counter_s));
	if (!hc) return NULL;

	hc->hc_num_buckets  = num_buckets;
	hc->hc_clock = hc->hc_buckets;
	return hc;
}

void dump_counters (hash_counter_s *hc)
{
	counter_s	*r = hc->hc_buckets;
	unsigned	n = hc->hc_num_buckets;
	unsigned	i;

	for (i = 0; i < n; i++, r++) {
		printf("%6u. %10u %d\n", i, r->c_key, r->c_count);
	}
}

void dump_buckets (hash_counter_s *hc)
{
	counter_s	*b = hc->hc_buckets;
	unsigned	n = hc->hc_num_buckets;
	counter_s	*r;
	unsigned	i;

	for (i = 0; i < n; i++, b++) {
		r = b->c_head;
		printf("%6u. ", i);
		for (r = b->c_head; r; r = r->c_next) {
			printf("%10u %5d ", r->c_key, r->c_count);
		}
		printf("\n");
	}
}

void dump_hcounter (hash_counter_s *hc)
{
	dump_counters(hc);
	dump_buckets(hc);
}

static inline counter_s *inc_clock (hash_counter_s *hc)
{
	counter_s	*r;

	++hc->hc_ticks;
	r = ++hc->hc_clock;
	if (r == &hc->hc_buckets[hc->hc_num_buckets]) {
		r = hc->hc_clock = hc->hc_buckets;
	}
	return r;
}

static inline counter_s *tick (hash_counter_s *hc)
{
	counter_s	*r;

	r = inc_clock(hc);
	if (r->c_key) {
		if (r->c_count < 0) {
			r->c_count = 0;
			return r;
		}
		r->c_count = -r->c_count;
		return NULL;
	}
	return r;
}

static counter_s *victim (hash_counter_s *hc)
{
	unsigned	h;
	counter_s	*b;
	counter_s	*r;
	counter_s	*v;
	counter_s	*prev;

	while ((v = tick(hc)) == NULL)
		;
	if (v->c_key) {
		h = HASH(hc, v->c_key);
		b = &hc->hc_buckets[h];
		r = b->c_head;
		if (r == v) {
			b->c_head = r->c_next;
		} else {
			for (;;) {
				prev = r;
				r = r->c_next;
				if (r == v) {
					prev->c_next = r->c_next;
					break;
				}
			}
		}
	}
	return v;
}

counter_s *hcounter (hash_counter_s *hc, unsigned x)
{
	unsigned	h = HASH(hc, x);
	counter_s	*b;
	counter_s	*r;
	counter_s	*prev;

	++hc->hc_total;
	b = &hc->hc_buckets[h];
	r = b->c_head;
	if (r) {
		if (r->c_key != x) {
			for (;;) {
				prev = r;
				r = r->c_next;
				if (!r) goto new;
				if (r->c_key == x) {
					prev->c_next = r->c_next;
					r->c_next = b->c_head;
					b->c_head = r;
					break;
				}
			}
		}
		if (r->c_count < 0) r->c_count = -r->c_count;
		++r->c_count;
		return r;
	}
new:
	r = victim(hc);
	r->c_next = b->c_head;
	b->c_head = r;
	r->c_key = x;
	r->c_count = 1;
	return r;
}

static unsigned cnt_find (hash_counter_s *hc, unsigned x)
{
	unsigned	h = HASH(hc, x);
	unsigned	i;
	counter_s	*r;

	r = hc->hc_buckets[h].c_head;
	for (i = 1; r; i++) {
		if (r->c_key == x) {
			return i;
		}
		r = r->c_next;
	}
	return 0x80000000;
}

void stat_hcounter (hash_counter_s *hc)
{
	counter_s	*r;
	unsigned	y;
	unsigned	n = 0;
	unsigned	max = 0;
	unsigned	sum = 0;
	unsigned	xmax = 0;
	int		nmax = 0;
	int		cnt;
	unsigned	nkey = 0;
	counter_s	*end = &hc->hc_buckets[hc->hc_num_buckets];

	for (r = hc->hc_buckets; r < end; r++) {
		if (r->c_key) {
			if (r->c_count < 0) {
				cnt = -r->c_count;
			} else {
				cnt = r->c_count;
			}
			if (cnt > nmax) {
				nmax = cnt;
				nkey = r->c_key;
			}
			y = cnt_find(hc, r->c_key);
			sum += y;
			++n;
			if (y > max) {
				max = y;
				xmax = r->c_key;
			}
		}
	}
	printf("n=%u ticks=%u total=%u"
		" t/t=%4.2f"
		" sum=%u avg=%4.2f max=%u nmax=%d\n",
		n, hc->hc_ticks, hc->hc_total,
		((double)hc->hc_ticks) / (double)(hc->hc_total),
		sum, (double)sum/(double)n, max, nmax);
}


#if 0
enum { M = 107, R = M };

int main (int argc, char *argv[])
{
	unsigned	i;
	unsigned	x;
	void		*hc;

	hc = init_hcounter(M, R);
	seed_random();
	for (i = 0; i < 2000*M; i++) {
#if 0
		unsigned	y;
		y = random() % (12*(M/3)) + 1;
		x = 1000000000/y +1;

		x = random() + 1;
#else
		x = exp_dist(200*M);
#endif
//printf("%u\n", x);
		if (!hcounter(hc, x)) break;
	}
	dump_hcounter(hc);
	stat_hcounter(hc);
	return 0;
}
#endif
