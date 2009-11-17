/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <qsort.h>

void *binarySearch (void *key, void **a, int n, cmp_f cmp, int *prev)
{
	int	low = 0;
	int	high = n-1;
	int	m, r;

	while (high >= low) {
		m = (low + high) / 2;
		r = cmp(key, a[m]);
		if (r < 0) {
			high = m - 1;
		} else if (r == 0) {
			return a[m];
		} else {
			low = m + 1;
		}
	}
	if (prev) *prev = high;
	return 0;
}

#if 0

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug.h>

char *A[] = { "b", "c", "e", "f", "j", "l", "m", "n", "o", "p", "s", "t", "u", "v" };

void good (char *a)
{
	char	*p;

	p = binarySearch(a, (void **)A, sizeof(A)/sizeof(char *), (cmp_f)strcmp, NULL);
	if (!p) {
		printf("Didn't find=%s\n", a);
		exit(1);
	}
}

void bad (char *a)
{
	char	*p;
	int		prev;

	p = binarySearch(a, (void **)A, sizeof(A)/sizeof(char *), (cmp_f)strcmp, &prev);
	if (p) {
		printf("shoudn't have found=%s\n", p);
		exit(1);
	}
PRd(prev);
	if (prev >= 0) {
		printf("%s<%s\n", A[prev], a);
	}
}

int main (int argc, char *argv[])
{
	int	i;

	for (i = 0; i < sizeof(A)/sizeof(char *); ++i) {
		good(A[i]);
	}
	good("s");
	good("b");
	good("v");
	bad("d");
	bad("a");
	bad("z");
	return 0;
}
/*
	good("b");
	good("v");
	bad("d");
	bad("a");
	bad("z");
*/
#endif
