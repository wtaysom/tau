/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
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

Lump_s prefixlump(Lump_s a, Lump_s b)
{
	Lump_s	p;
	int	size;
	int	i;
	char	*ap = a.d;
	char	*bp = b.d;

	if (a.size > b.size) {
		size = b.size;
	} else {
		size = a.size;
	}
	if (!ap || !bp) {
		return Nil;
	}
	for (i = 0; i < size; i++) {
		if (ap++ != bp++) break;
	}
	p.size = i;
	p.d = a.d;
	return p;
}

void freelump(Lump_s a)
{
	if (a.d) free(a.d);
	a.d = NULL;
}

bool prlump(const char *fn, unsigned line, const char *label, Lump_s a)
{
	enum { MAX_LUMP = 32 };
	char	buf[MAX_LUMP+1];
	int	i;
	int	size;
	char	*cp;

	size = a.size;
	if (size > MAX_LUMP) size = MAX_LUMP;
	for (i = 0; i < size; i++) {
		cp = a.d;
		if (isprint(cp[i])) {
			buf[i] = cp[i];
		} else {
			buf[i] = '.';
		}
	}
	buf[size] = '\0';
	return print(fn, line, "%s=%d:%s", label, a.size, buf);
}
