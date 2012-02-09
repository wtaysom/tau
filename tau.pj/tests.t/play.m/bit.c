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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bit.h>
#include <debug.h>
#include <eprintf.h>

unsigned simpleFindHighBit (unsigned long x)
{
	unsigned	cnt;

	for (cnt = 0; x != 0; ++cnt, x >>= 1)
		;
	return cnt;
}

unsigned simpleLowBit (unsigned long x)
{
	unsigned	cnt;

	if (x == 0) return 0;
	for (cnt = 1; (x & 1) == 0; ++cnt, x >>= 1)
		;
	return cnt;
}

void tstFindHighBit (unsigned long x)
{
	unsigned	a;
	unsigned	b;

	a = simpleFindHighBit(x);
	b = flsl(x);
	if (a != b) {
		fatal("%lx %d %d", x, a, b);
	}
}

void tstLowBit (unsigned long x)
{
	unsigned	a;
	unsigned	b;

	a = simpleLowBit(x);
	b = ffsl(x);
	if (a != b) {
		fatal("%lx %x %x", x, a, b);
	}
}

void tstRandX (void (*tst)(unsigned long x))
{
	int	i;

	for (i = 0; i < 1000; ++i) {
		tst(rand());
	}
}

int main (int argc, char *argv[])
{
	tstFindHighBit(0);
	tstFindHighBit(1);
	tstFindHighBit(2);
	tstFindHighBit(3);
	tstFindHighBit(-1);
	tstFindHighBit(37);
	tstFindHighBit(0x80000000);
	tstLowBit(0);
	tstLowBit(1);
	tstLowBit(2);
	tstLowBit(3);
	tstLowBit(-1);
	tstLowBit(37);
	tstLowBit(0x80000000);
	tstRandX(tstFindHighBit);
	tstRandX(tstLowBit);
	return 0;
}
