/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eprintf.h>
#include <style.h>

enum {	MAX_RECS = 4,
	NUM_BUCKETS = 17 };

typedef struct Rec_s {
	u32	key;
	u32	val;
} Rec_s;

typedef struct Ant_s	Ant_s;
struct Ant_s {
	Ant_s	*next;
	u32	id;
	u16	level;
	u16	n;
	Rec_s	r[MAX_RECS];
};
	
u32 Root;

u32 genkey (void)
{
	static u32	key = 0;
	return ++key;
}

u32 genval (void)
{
	static u32	val = 7;
	return ++val;
}

static Ant_s	*Bucket[NUM_BUCKETS];

static Ant_s *hash (u32 id)
{
	return (Ant_s *)&Bucket[id % NUM_BUCKETS];
}

Ant_s *find_ant (u32 id)
{
	Ant_s	*a = hash(id)->next;

	while (a) {
		if (id == a->id) break;
		a = a->next;
	}
	return a;
}

void delete_ant (u32 id)
{
	Ant_s	*prev = hash(id);
	Ant_s	*a = prev->next;

	while (a) {
		if (id == a->id) {
			prev->next = a->next;
			free(a);
			return;
		}
		prev = a;
		a = a->next;
	}
	fatal("Not found %ld\n", id);
}

void add_ant (Ant_s *a)
{
	Ant_s	*prev = hash(a->id);

	a->next = prev->next;
	prev->next = a;
}

Ant_s *new_ant (void)
{
	static u32	id = 0;

	Ant_s	*a = ezalloc(sizeof(*a));
	a->id = ++id;
	add_ant(a);
	return a;
}

void split (Ant_s *a)
{
	fatal("Not implemented");
}

void insert (u32 id, Rec_s r)
{
	u32	i;

	if (a == 0) {
		a = new_ant();
		Root = a->id;
	} else {
		a = find_ant(id);
	}
	if (level < a->level) {
		for (i = 0; i < a->n; i++) {
			if (r.key < a->r[i].key) {
				insert(a->r[i].val, level, r);
				return;
			}
		}
	}
	if (a->n == MAX_RECS) {
		split(a);
		insert(Root, r);
		return;
	}
	for (i = 0; i < a->n; i++) {
		if (r.key < a->r[i].key) {
			memmove( &a->r[i+1], &a->r[i], a->n - i);
			break;
		}
	}
	a->r[i] = r;
	++a->n;
}

int main (int argc, char *argv[])
{
	Rec_s	r;
	u32	i;

	for (i = 0; i < 10; i++) {
		r.key = genkey();
		r.val = genval();
		insert(Root, r);
	}
	return 0;
}
