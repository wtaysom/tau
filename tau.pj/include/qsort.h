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

#ifndef _QSORT_H_
#define _QSORT_H_

/*
 * Sort algorithms based on Robert Sedgewick, "Algorithms in C."
 * Insertion sort is used as the final phase of quick sort but
 * can be used independently for small files or files that are
 * nearly sorted.
 *
 * These sort routines expect three arguments:
 *	void *a[]     : an a array of pointers to the items to be sorted.
 *	int   n       : the number of pointers in the array a.
 *	cmpfunc_t cmp : function that compares two items from a.
 */

	/*
	 * Function used by the sort routines to compare two items being
	 * sorted.  Must return a signed integer:
	 *	<0 : a is less than b
	 *	 0 : a is equal to b
	 *	>0 : a is greater than b
	 */
typedef int (*cmp_f)(const void *a, const void *b);
typedef void **vector_t;

void insertionSort(void **a, int n, cmp_f cmp);
void quickSort(void **a, int n, cmp_f cmp);
void *binarySearch(void *key, void **a, int n, cmp_f cmp, int *prev);

#endif
