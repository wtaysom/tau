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
#include <stdlib.h>
#include <stdio.h>

#include <mylib.h>
#include <eprintf.h>

#if 1
#define swap(_x, _y) {	\
	int	_t;	\
	_t = _y;	\
	_y = _x;	\
	_x = _t;	\
}
#else
#define swap(_x, _y) {	\
	_x ^= _y;	\
	_y ^= _x;	\
	_x ^= _y;	\
}
#endif

void pr (int *a, int n)
{
	int	i;

	for (i = 0; i < n; i++) {
		printf("%3d\n", *a++);
	}
}

int partition (int *a, int n)
{
	int	i;
	int	m;

	m = 0;
	for (i = 1; i < n; i++) {
		if (a[i] < a[0]) {
			++m;
			if (i > m) {
				swap(a[i], a[m]);
			}
		}
	}
	swap(a[0], a[m]);
	return m;
}

void q_sort (int *a, int n)
{
	int	m;

	if (n < 2) return;
	m = partition(a, n);
	q_sort(a, m);
	q_sort( &a[m+1], n - m - 1);
}

int main (int argc, char *argv[])
{
	int	n = 10;
	int	i;
	int	*a;

	if (argc > 1) {
		n = atoi(argv[1]);
	}
	seed_random();
	a = emalloc(n * sizeof(int));
	for (i = 0; i < n; i++) {
		a[i] = range(100);
	}
	pr(a, n);
	printf("\n");
	q_sort(a, n);
	pr(a, n);
	return 0;
}
