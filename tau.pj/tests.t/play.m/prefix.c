/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

#include <lump.h>
#include <mystdlib.h>

char *rndstring (unint n) {
	char	*s;
	unint	j;

	if (!n) return NULL;

	s = malloc(n);
	for (j = 0; j < n-1; j++) {
		s[j] = 'a' + urand(3);
	}
	s[j] = 0;
	return s;
}

Lump_s rnd_lump (void) {
	unint	n;

	n = urand(2) + 2;
	return lumpmk(n, rndstring(n));
}

Lump_s fixed_lump (unint n) {
	return lumpmk(n, rndstring(n));
}

Lump_s prefix(Lump_s a, Lump_s b) {
	Lump_s	p;
	int	size;
	int	i;

#if 0
	int	cmp;
	cmp = cmplump(a, b);
	if (cmp > 0) {
		Lump_s	t = a;
		a = b;
		b = t;
	} else if (cmp == 0) {
		return a;
	}
#endif
	if (a.size > b.size) {
		size = b.size;
	} else {
		size = a.size;
	}
	for (i = 0; i < size; i++) {
		if (((u8 *)a.d)[i] != ((u8 *)b.d)[i]) {
			++i;
			break;
		}
	}
	p.size = i;
	p.d = b.d;
	return p;
}

#define LUMP(_x)	(_x).size, ((u8 *)(_x).d)
int main (int argc, char *argv[]) {
	Lump_s	a;
	Lump_s	b;
	Lump_s	p;
	int	i;

	for (i = 0; i < 20; i++) {
		a = rnd_lump();
		b = rnd_lump();
		p = prefix(a, b);
		printf("%.*s %.*s %.*s\n", LUMP(a), LUMP(b), LUMP(p));
	}
	return 0;
}
