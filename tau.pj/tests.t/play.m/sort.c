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

#include <style.h>
#include <mylib.h>
#include <eprintf.h>
#include <timer.h>

#if 1
static inline void swap (int *x, int *y)
{
	int	t;
	t  = *y;
	*y = *x;
	*x = t;
}
#else
static inline void swap (int *x, int *y)
{
	*x ^= *y;
	*y ^= *x;
	*x ^= *y;
}
#define swap(_x, _y) {	\
	int	_t;	\
	_t = _y;	\
	_y = _x;	\
	_x = _t;	\
}
#define swap(_x, _y) {	\
	_x ^= _y;	\
	_y ^= _x;	\
	_x ^= _y;	\
}
if (i != m) swap( &a[i], &a[m]);
if (m) swap( &a[0],  &a[m]);
#endif

void pr (int *a, int n)
{
	int	i;

	for (i = 0; i < n; i++) {
		printf("%3d\n", *a++);
	}
}

int *partition (int *a, int *u)
{
	int	b;
	int	*l = a;
	int	*m = a;

	b = *l++;
	for (; l < u; l++) {
		if (*l < b) {
			++m;
			swap(l, m);
		}
	}
	swap(a,  m);
	return m;
}

void q_sort (int *a, int *u)
{
	int	*m;

	//if (u - a < 2) return;
	if (a == u) return;
	m = partition(a, u);
	q_sort(a, m);
	q_sort(m + 1, u);
}

int main (int argc, char *argv[])
{
	int	n = 10;
	int	i;
	int	*a;
	u64	startTime, endTime;
	u64	delta;

	if (argc > 1) {
		n = atoi(argv[1]);
	}
	seed_random();
	a = emalloc(n * sizeof(int));
	for (i = 0; i < n; i++) {
		a[i] = urand(100000);
	}
	//pr(a, n);
	printf("\n");
	startTime = nsecs();
	q_sort(a, &a[n]);
	endTime = nsecs();
	delta = endTime - startTime;
	printf("diff=%lld nsecs per element=%g\n", delta, (double)delta/n);
	//pr(a, n);
	return 0;
}
