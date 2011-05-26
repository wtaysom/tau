/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

#include <lump.h>
#include <mylib.h>

char *rndstring (unint n) {
	char	*s;
	unint	j;

	if (!n) return NULL;

	s = malloc(n);
	for (j = 0; j < n-1; j++) {
		s[j] = 'a' + range(26);
	}
	s[j] = 0;
	return s;
}

Lump_s rnd_lump (void) {
	unint	n;

	n = range(7) + 5;
	return lumpmk(n, rndstring(n));
}

Lump_s fixed_lump (unint n) {
	return lumpmk(n, rndstring(n));
}

#define LUMP(_x)	(_x).size, (_x).d
int main (int argc, char *argv[]) {
	Lump_s	a;
	Lump_s	b;
	Lump_s	p;
	int	i;

	for (i = 0; i < 10; i++) {
		a = rnd_lump();
		b = rnd_lump();
		p = prefixlump(a, b);
		printf("%.*s %.*s %.*s\n", LUMP(a), LUMP(b), LUMP(p));
	}
	return 0;
}
