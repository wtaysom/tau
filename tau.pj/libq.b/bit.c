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

#include "bit.h"

unsigned ffsBit (register unsigned long word)
{
	register unsigned	bit;
	register unsigned long	tmp;

	bit = 0;
	if (sizeof(word) == 8) {
		tmp = word >> (sizeof(word) * 4);
		if (tmp) {
			bit += 32;
			word = tmp;
		}
	}
	tmp = word >> 16;
	if (tmp) {
		bit += 16;
		word = tmp;
	}
	tmp = word >> 8;
	if (tmp) {
		bit += 8;
		word = tmp;
	}
	tmp = word >> 4;
	if (tmp) {
		bit += 4;
		word = tmp;
	}
	tmp = word >> 2;
	if (tmp) {
		bit += 2;
		word = tmp;
	}
	bit += (word >> 1);

	return bit;
}

#ifdef TESTBIT
unsigned simpleFindHighBit (unsigned long x)
{
	unsigned	cnt;

	if (x == 0) return 0;
	for (cnt = 0; x != 0; ++cnt, x >>= 1)
		;
	return cnt - 1;
}

unsigned simpleLowBit (unsigned long x)
{
	unsigned	cnt = 0;

	if (x == 0) return 0;
	for (cnt = 0; (x & 1) == 0; ++cnt, x >>= 1)
		;
	return 1 << cnt;
}

void tstFindHighBit (unsigned long x)
{
	if (simpleFindHighBit(x) != ffsBit(x)) {
		printf("FAILED FindHighBit: %x %d %d\n",
			x, simpleFindHighBit(x), ffsBit(x));
	}
}

void tstLowBit (unsigned long x)
{
	if (simpleLowBit(x) != lowBit(x)) {
		printf("FAILED lowBit: %x %x %x\n",
			x, simpleLowBit(x), lowBit(x));
	}
}

void tstRandX (void (*tst)())
{
	int	i;

	for (i = 0; i < 1000; ++i) {
		tst(rand());
	}
}

main ()
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
}
#endif
