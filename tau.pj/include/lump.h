/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#ifndef _LUMP_H_
#define _LUMP_H_ 1

#include <debug.h>
#include <style.h>

typedef struct Lump_s {
	int size;
	void *d;
} Lump_s;

typedef struct Rec_s {
	Lump_s key;
	Lump_s val;
} Rec_s;

extern const Lump_s Nil;

int    cmplump(Lump_s a, Lump_s b);
Lump_s duplump(Lump_s a);
Lump_s copylump(Lump_s a, int n, void *buf);
Lump_s prefixlump(Lump_s a, Lump_s b);
void   freelump(Lump_s a);

static inline Lump_s lumpmk(int size, void *data)
{
	Lump_s a;

	a.size = size;
	a.d = data;
	return a;
}

bool prlump(const char *fn, unsigned line, const char *, Lump_s x);

#define PRlp(_x_)	prlump(FN_ARG, # _x_, _x_)

#endif
