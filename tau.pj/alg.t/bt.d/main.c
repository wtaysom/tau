/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <stdlib.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mylib.h>
#include <style.h>

#include "bt.h"

enum {	NUM_BUFS = 10 };

char *rndstring(unint n)
{
	char *s;
	unint j;

	if (!n) return NULL;

	s = malloc(n);
	for (j = 0; j < n-1; j++) {
		s[j] = 'a' + range(26);
	}
	s[j] = 0;
	return s;
}

Lump_s rnd_lump(void)
{
	unint n;

	n = range(7) + 5;
	return lumpmk(n, rndstring(n));
}

enum { NUM_BUCKETS = 7 };

typedef void (*recFunc)(Lump_s key, Lump_s val, void *user);

struct Record {
	struct Record *next;
	Lump_s key;
	Lump_s val;
};

struct Record *Bucket[NUM_BUCKETS];

static struct Record *hash(Lump_s key)
{
	unint h = crc32(key.d, key.size) % NUM_BUCKETS;
	return (struct Record *)&Bucket[h];
}

static void h_add(Lump_s key, Lump_s val)
{
	struct Record *h;
	struct Record *r;

	h = hash(key);
	r = malloc(sizeof(*r));
	r->key = key;
	r->val = val;
	r->next = h->next;
	h->next = r;
}

#if 0
static Lump_s h_find(Lump_s key)
{
FN;
	struct Record *h;
	struct Record *r;

	h = hash(key);
	r = h->next;
	while (r) {
		if (lumpcmp(key, r->key) == 0) {
			return r->val;
		}
		r = r->next;
	}
	return Nil;
}
#endif

static void h_for_each (recFunc f, void *user)
{
FN;
	unint i;
	struct Record *r;

	for (i = 0; i < NUM_BUCKETS; i++) {
		r = Bucket[i];
		while (r) {
			f(r->key, r->val, user);
		}
	}
}

static void print_f(Lump_s key, Lump_s val, void *t)
{
FN;
	Lump_s v;
	v = t_find(t, key);
	printf("%s=%s", key.d, val.d);
	if (lumpcmp(val, v) != 0) {
		printf("!=%s\n", v.d);
	} else {
		printf("\n");
	}
	lumpfree(v);
}

void t_print(Btree_s *t)
{
FN;
	h_for_each(print_f, t);
}

static void test4(void)
{
FN;
	Btree_s *t;
	Lump_s key;
	Lump_s val;
	unint i;

	if (FALSE) seed_random();
	t = t_new(".tfile", NUM_BUFS);
	for (i = 0; i < 10/*23*/; i++) {
		key = rnd_lump();
		val = rnd_lump();
		h_add(key, val);
		t_insert(t, key, val);
	t_dump(t);
	}
}

int main(int argc, char *argv[])
{
FN;
	fdebugoff();
	test4();
	return 0;
}
