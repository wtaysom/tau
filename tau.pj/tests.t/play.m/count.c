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

enum {	EXPONENT = 5,
	INTEGER = 8 - EXPONENT,
	INTEGER_MASK = (1 << INTEGER) - 1 };

typedef u8 f8;

static inline unint integer (f8 x) { return x & INTEGER_MASK; }
static inline unint exponent (f8 x) { return x >> INTEGER; }

unint f8_to_unint (f8 x)
{
	if (!x) return 0;
	unint	e = exponent(x);
	unint	i = integer(x);

	if (e) {
		return (i + (1 << INTEGER)) << (e - 1);
	} else {
		return i;
	}
}

f8 unint_to_f8 (unint x)
{
	if (x < (1 << (1 + INTEGER))) return x;
	
	unint	e = flsl(x) - INTEGER - 1;
	unint	i = x >> e;
printf("e=%lu i=%lu ", e, i);
	return (e << (INTEGER + 1)) | i;
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
