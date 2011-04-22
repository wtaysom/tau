/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <string.h>

#include <lump.h>

const Lump_s Nil = { -1, NULL };

int cmplump(Lump_s a, Lump_s b)
{
	int	size;
	int	x;

	if (a.size > b.size) {
		size = b.size;
	} else {
		size = a.size;
	}
	x = memcmp(a.d, b.d, size);
	if (x) return x;
	if (a.size < b.size) return -1;
	if (a.size == b.size) return 0;
	return 1;
}

Lump_s duplump(Lump_s a)
{
	Lump_s	b;

	b.d = malloc(a.size);
	b.size = a.size;
	memmove(b.d, a.d, a.size);
	return b;
}

void freelump(Lump_s a)
{
	if (a.d) free(a.d);
}

