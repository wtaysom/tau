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

/*
 * Finds the first bit counting from the top or the highest bit set.
 * I number bits 0-31, ffs and fls number bits 1-32 so 0 is no bits set.
 * This is the oposite of ffs in string.h
 *
 * ffs - finds the least significant bit set (find first bit set)
 * fls - find the most significant bit set (find last bit set)
 *
 * Coverted my code to work the same as ffs and fls. (I would use them except
 * fls is not always supported).
 */

#ifdef __APPLE__

char must_have_at_least_one_symbol_defined;

#else

unsigned flsl (register unsigned long word)
{
	register unsigned	bit;
	register unsigned long	tmp;

	if (!word) return 0;
	bit = 1;
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

#endif
