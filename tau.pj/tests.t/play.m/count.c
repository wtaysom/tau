/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>

#include <bit.h>
#include <debug.h>
#include <eprintf.h>
#include <style.h>
#include <twister.h>

/*
 * Implement 8 bit floating point representation. It just represents
 * large numbers, not decimals. This is to store the history of compact
 * counters.
 * +--------+-------+
 * |exponent|integer|
 * +--------+-------+
 * integer << exponent
 */

enum {	INT_SHIFT = 3,
	INT_MASK = (1 << INT_SHIFT) - 1,
	INT_ADD = 1 << INT_SHIFT };

typedef u8 f8;

static inline unint integer (f8 x) { return x & INT_MASK; }
static inline unint exponent (f8 x) { return x >> INT_SHIFT; }

unint f8_to_unint (f8 x)
{
	if (!x) return 0;
	unint	e = exponent(x);
	unint	i = integer(x);

	if (e) {
		return (i + INT_ADD) << (e - 1);
	} else {
		return i;
	}
}

f8 unint_to_f8 (unint x)
{
	if (x < (INT_ADD << 1)) return x;
	
	unint	b = flsl(x) - 1;
	unint	i = x >> (b - INT_SHIFT) & INT_MASK;
	unint	e = b - INT_SHIFT + 1;
printf("e=%lu i=%lu ", e, i);
	return (e << INT_SHIFT) | i;
}

int main (int argc, char *argv[])
{
	unint	x;

	for (x = 0; x < 256; x++) {
		unint y = f8_to_unint(x);
		unint z = unint_to_f8(y);
		printf("%lu %lu %lu\n", x, y, z);
//		printf("%lu %lu\n", x, z);
	}
	return 0;
}
