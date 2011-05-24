/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <lump.h>
#include <debug.h>

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
	if (!a.d) {
		if (b.d) return -1;
		else return 0;
	} else {
		if (!b.d) return 1;
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

Lump_s copylump(Lump_s a, int n, void *buf)
{
	Lump_s	b;

	if (n < a.size) {
		return Nil;
	}
	b.d = buf;
	b.size = a.size;
	memmove(b.d, a.d, a.size);
	return b;
}

void freelump(Lump_s a)
{
	if (a.d) free(a.d);
}

bool prlump(const char *fn, unsigned line, const char *label, Lump_s a)
{
	enum { MAX_LUMP = 32 };
	char	buf[MAX_LUMP+1];
	int	i;
	int	size;

	size = a.size;
	if (size > MAX_LUMP) size = MAX_LUMP;
	for (i = 0; i < size; i++) {
		if (isprint(a.d[i])) {
			buf[i] = a.d[i];
		} else {
			buf[i] = '.';
		}
	}
	buf[size] = '\0';
	return print(fn, line, "%s=%d:%s", label, a.size, buf);
}

char *strlump(Lump_s a)
{
	enum { MAX_LUMP = 32 };
	static char	buf[MAX_LUMP+1];
	int	i;
	int	size;

	size = a.size;
	if (size > MAX_LUMP) size = MAX_LUMP;
	for (i = 0; i < size; i++) {
		if (isprint(a.d[i])) {
			buf[i] = a.d[i];
		} else {
			buf[i] = '.';
		}
	}
	buf[size] = '\0';
	return buf;
}
