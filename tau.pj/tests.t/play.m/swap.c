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

#include <mystdlib.h>
#include <eprintf.h>

#define swap(_x, _y) {	\
	_x ^= _y;	\
	_y ^= _x;	\
	_x ^= _y;	\
}

void tswap (int a, int b)
{
	printf("%4d %4d\n", a, b);
	swap(a,b);
	printf("%4d %4d\n\n", a, b);
}

int main (int argc, char *argv[])
{
	int	n = 10;
	int	i;

	if (argc > 1) {
		n = atoi(argv[1]);
	}
	for (i = 0; i < n; i++) {
		tswap(urand(10), urand(10));
		tswap(i, i);
	}
	return 0;
}
