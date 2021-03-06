/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <stdlib.h>

#include <eprintf.h>
#include <lump.h>
#include <mystdlib.h>
#include <twister.h>

#include "util.h"

void Pause(void)
{
	printf(": ");
	getchar();
}

char *rndstring(unint n)
{
	char *s;
	unint j;

	if (!n) return NULL;

	s = malloc(n);
	for (j = 0; j < n-1; j++) {
		s[j] = 'a' + twister_urand(26);
	}
	s[j] = 0;
	return s;
}

Lump_s rnd_lump(void)
{
	unint n;

	n = twister_urand(7) + 5;
	return lumpmk(n, rndstring(n));
}

Lump_s fixed_lump(unint n)
{
	return lumpmk(n, rndstring(n));
}

Lump_s seq_lump(void)
{
	enum { MAX_KEY = 4 };

	static int n = 0;
	int x = n++;
	char *s;
	int i;
	int r;

	s = malloc(MAX_KEY);
	i = MAX_KEY - 1;
	s[i]   = '\0';
	do {
		--i;
		r = x % 26;
		x /= 26;
		s[i] = 'a' + r;
	} while (x && (i > 0));
	while (--i >= 0) {
		s[i] = ' ';
	}
	return lumpmk(MAX_KEY, s);
}


enum {	DYNA_START  = (1<<10),
	DYNA_MAX    = 1 << 27 };

static u64 *K;
static u64 *Next;
static unint Size;

void k_init (void)
{
	if (K) {
		free(K);
	}
	Size = DYNA_START;
	Next = K = emalloc(Size * sizeof(*K));
}

void k_add (u64 key)
{
	assert(K);
	if (Next == &K[Size]) {
		unint  newsize;

		if (Size >= DYNA_MAX) {
			fatal("Size %ld > %d", Size, DYNA_MAX);
		}
		newsize = Size << 2;
		K = erealloc(K, newsize * sizeof(*K));
		Next = &K[Size];
		Size = newsize;
	}
	*Next++ = key;
}

void k_for_each (krecFunc f, void *user) {
	u64 *k;

	for (k = K; k < Next; k++) {
		f(*k, user);
	}
}

snint k_rand_index (void)
{
	snint x = Next - K;

	if (!K) return -1;
	if (x == 0) return -1;
	return twister_urand(x);
}

u64 k_get_rand (void) {
	snint x = k_rand_index();

	if (x == -1) {
		return 0;
	}
	return K[x];
}

u64 k_delete_rand (void)
{
	snint x = k_rand_index();
	u64 key;

	if (x == -1) return 0;
	key = K[x];
	K[x] = *--Next;
	return key;
}

int k_should_delete(s64 count, s64 level)
{
	enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
	return (twister_random() & MASK) * count / level / RANGE;
}

static u64 Key = 1;

void k_seed (u64 seed)
{
	srandom(seed);
	Key = seed;
}

u64 k_rand_key (void)
{
	return twister_random();
#if 0
	return Key++;
	return twister_urand(10000);
	return twister_random();
#endif
}

bool k_rand_percent (unint percent)
{
	return twister_urand(100) < percent;
}

static Rec_s *R;
static Rec_s *Next_rec;
static unint Size;

void r_add (Rec_s rec)
{
	Rec_s *r;

	if (!R) {
		Size = DYNA_START;
		Next_rec = R = emalloc(Size * sizeof(*R));
	}
	if (Next_rec == &R[Size]) {
		unint  newsize;

		if (Size >= DYNA_MAX) {
			fatal("Size %ld > %d", Size, DYNA_MAX);
		}
		newsize = Size << 2;
		R = erealloc(R, newsize * sizeof(*R));
		Next_rec = &R[Size];
		Size = newsize;
	}
	r = Next_rec++;
	*r = rec;
}

snint r_rand_index (void)
{
	snint x = Next_rec - R;

	if (!R) return -1;
	if (x == 0) return -1;
	return twister_urand(x);
}

void r_for_each (recFunc f, void *user)
{
	Rec_s *r;

	for (r = R; r < Next_rec; r++) {
		f(r->key, r->val, user);
	}
}

Rec_s r_get_rand (void)
{
	snint x = r_rand_index();

	if (x == -1) {
		Rec_s r = { Nil, Nil };
		return r;
	}
	return R[x];
}

Lump_s r_delete_rand (void)
{
	snint x = r_rand_index();
	Lump_s key;

	if (x == -1) return Nil;
	key = R[x].key;
	R[x] = *--Next_rec;
	return key;
}
