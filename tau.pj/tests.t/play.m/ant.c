/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include <eprintf.h>
#include <style.h>

enum { MAX_RECS = 4 };

typedef struct Rec_s {
	u32	key;
	u32	val;
} Rec_s;

typedef struct Ant_s {
	u16	level;
	u16	n;
	Rec_s	r[MAX_RECS];
} Ant_s;
	
Ant_s	*Root;

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

Ant_s *new_ant (void)
{
	Ant_s	*a = ezalloc(sizeof(*a));
	return a;
}

void split (Ant_s *a)
{
	fatal("Not implemented");
}

void insert (Ant_s *a, Rec_s r)
{
	u32	i;

	if (a == NULL) {
		Root = a = new_ant();
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
