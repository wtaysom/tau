/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _LUMP_H_
#define _LUMP_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _DEBUG_H_
#include <debug.h>
#endif

typedef struct Lump_s {
	int size;
	u8 *d;
} Lump_s;

extern const Lump_s Nil;

int    cmplump(Lump_s a, Lump_s b);
Lump_s duplump(Lump_s a);
Lump_s copylump(Lump_s a, int n, void *buf);
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
