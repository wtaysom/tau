/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BT_H_
#define _BT_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct Btree_s Btree_s;

typedef struct Lump_s {
	int size;
	u8 *d;
} Lump_s;

extern const Lump_s Nil;

int    lumpcmp(Lump_s a, Lump_s b);
Lump_s lumpdup(Lump_s a);
void   lumpfree(Lump_s a);

static inline Lump_s lumpmk(int size, void *data)
{
	Lump_s b;

	b.size = size;
	b.d = data;
	return b;
}

Btree_s *t_new(char *file, int num_bufs);
void     t_dump(Btree_s *t);
int      t_insert(Btree_s *t, Lump_s key, Lump_s val);
Lump_s   t_find(Btree_s *t, Lump_s key);

#endif
