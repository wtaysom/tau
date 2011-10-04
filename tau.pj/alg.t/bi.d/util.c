/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>

#include <lump.h>
#include <mystdlib.h>

#include "util.h"

char *rndstring(unint n)
{
	char *s;
	unint j;

	if (!n) return NULL;

	s = malloc(n);
	for (j = 0; j < n-1; j++) {
		s[j] = 'a' + urand(26);
	}
	s[j] = 0;
	return s;
}

Lump_s rnd_lump(void)
{
	unint n;

	n = urand(7) + 5;
	return lumpmk(n, rndstring(n));
}

Lump_s fixed_lump(unint n)
{
	return lumpmk(n, rndstring(n));
}

Lump_s seq_lump(void)
{
	enum { MAX_KEY = 4 };

	static int n = 0;
	int x = n++;
	char *s;
	int i;
	int r;

	s = malloc(MAX_KEY);
	i = MAX_KEY - 1;
	s[i]   = '\0';
	do {
		--i;
		r = x % 26;
		x /= 26;
		s[i] = 'a' + r;
	} while (x && (i > 0));
	while (--i >= 0) {
		s[i] = ' ';
	}
	return lumpmk(MAX_KEY, s);
}
