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

/*
 * Quick sort algorithm based on Robert Sedgewick, "Algorithms in C."
 */

#include <qsort.h>

enum {
	qMIN_PARTITION = 11,	/* This seems optimal (by a fraction of a percent).
				 * The sort time is relativily insenitive over a
				 * range of values (1 - 23).
				 */
};

void insertionSort (void *a[], int n, cmp_f cmp)
{
	void	**scan;
	void	**insert;
	void	*current;

	for (scan = &a[1]; scan < &a[n]; ++scan)
	{
		current = scan[0];
		for (insert = scan;
			/* put in insert compare so we don't need a sentinal. */
			(insert > a) && cmp(insert[-1], current) > 0;
			--insert)
		{
			*insert = insert[-1];
		}
		*insert = current;
	}
}

#define qSWAP(_a, _b)		\
{				\
	void	*tmp;		\
				\
	tmp = (_a);		\
	(_a) = (_b);		\
	(_b) = tmp;		\
}

#define qSORT3(_x, _y, _z)			\
{						\
	if (cmp((_x), (_y)) > 0) qSWAP(_x, _y);	\
	if (cmp((_x), (_z)) > 0) qSWAP(_x, _z);	\
	if (cmp((_y), (_x)) > 0) qSWAP(_y, _z);	\
}

#define qPUSH(_x)	(*top++ = (_x))
#define qPOP()		(*--top)
#define qEMPTY()	(top == stack)

void quickSort (void *a[], int n, cmp_f cmp)
{
	int	i;
	int	left;
	int	right;
	int	stack[sizeof(int) * 8 * 2];
	int	*top = stack;

	left  = 1;
	right = n - 1;
	for (;;)
	{
		while ((right - left) > qMIN_PARTITION)
		{
				/*
				 * partition
				 */
			void	*current;
			int	j;
			int	median = ((right - left) / 2) + left;

			qSORT3(a[left], a[median], a[right]);
			qSWAP(a[median], a[right - 1]);

			current = a[right - 1];
			i = left - 2;
			j = right - 1;
			for (;;)
			{
				while (cmp(a[++i], current) < 0)
					;
				while (cmp(a[--j], current) > 0)
					;
				if (i >= j)
				{
					break;
				}
				qSWAP(a[i], a[j]);
			}
			qSWAP(a[i], a[right - 1]);
				/*
				 * recursion
				 */
			if (i - left > right - i)
			{
				qPUSH(left); qPUSH(i - 1);
				left = i + 1;
			}
			else
			{
				qPUSH(i + 1); qPUSH(right);
				right = i - 1;
			}
		}
		if (qEMPTY())
		{
			break;
		}
		right = qPOP();
		left  = qPOP();
	}
	insertionSort(a, n, cmp);
}

#define QUICKSORT_TESTING 1
#ifdef QUICKSORT_TESTING

#include <stdlib.h>
#include <stdio.h>

#include <style.h>
#include <timer.h>

typedef unsigned long long	quad;

typedef struct Struct_s
{
	quad	key;
	int	data;
} Struct_s;

	/*
	 * If the key were an int, we could do a simple
	 * subtraction.
	 */
int cmpStruct (const Struct_s *a, const Struct_s *b)
{
/*
	return b->key - a->key;
*/
	if (a->key > b->key) return 1;
	if (a->key < b->key) return -1;
	return 0;
}

int main (int argc, char *argv[])
{
	Struct_s	**a;
	Struct_s	*data;
	int		i, n;
	int		print, rnd;
	u64		startTime, endTime;
	u64		delta;

	if (argc > 1) {
		n = atoi(argv[1]);
	} else {
		n = 60;
	}
	if (argc > 2) {
		print = (*argv[2] == 'p');
	} else {
		print = 1;
	}
	if (argc > 3) {
		rnd = (*argv[3] == 'r');
	} else {
		rnd   = 0;
	}
	data = malloc(sizeof(Struct_s) * n);
	a = malloc(sizeof(Struct_s *) * n);
	for (i = 0; i < n; ++i) {
		if (rnd) data[i].key = random() % n;
		else data[i].key = n - i;
		data[i].data = i;
		a[i] = &data[i];
	}
	startTime = nsecs();
	quickSort((void **)a, n, (cmp_f)cmpStruct);
	endTime = nsecs();

	delta = endTime - startTime;
	printf("diff=%lld nsecs per element=%g\n", delta, (double)delta/n);

#if 0
	for (i = 0; print && (i < n); ) {
		printf("%10qd %8d", a[i]->key, a[i]->data);
		if (++i % 4) printf(" ");
		else printf("\n");
	}
	if (n % 4) printf("\n");
#endif
	return 0;
}

#if 0

int	N;
int	*A;

pra ()
{
	int	i;

	for (i = 0; i < N;) {
		printf("%10d", A[i]);
		if (++i % 6) printf(" ");
		else printf("\n");
	}
	if (N % 6) printf("\n");
}
printf("partition l=%d r=%d\n", left, right);
printf("qsort l=%d r=%d\n", l, r);
pra();
printf("left=%d right=%d median=%d\n", left, right, median);
printf("a[left]=%d a[right]=%d a[median]=%d\n", a[left], a[right], a[median]);
printf("a[left]=%d a[right]=%d a[median]=%d\n", a[left], a[right], a[median]);
printf("left=%d right=%d diff=%d\n", left, right, right - left);
#endif

#endif

