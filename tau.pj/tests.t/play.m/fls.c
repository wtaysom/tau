/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <strings.h>

unsigned flsl (unsigned long x)
{
	unsigned	b = 1;
	unsigned long	t;

	if (sizeof(x) == 8) {
		t = x >> (sizeof(x) * 4);
		if (t) {
			b += 32;
			x = t;
		}
	}
	if (!x) return 0;
	t = x >> 16;
	if (t) {
		b += 16;
		x = t;
	}
	t = x >> 8;
	if (t) {
		b += 8;
		x = t;
	}
	t = x >> 4;
	if (t) {
		b += 4;
		x = t;
	}
	t = x >> 2;
	if (t) {
		b += 2;
		x = t;
	}
	b += (x >> 1);
	return b;
}

unsigned long fib (unsigned long i)
{
	unsigned long	f1;
	unsigned long	f2;
	unsigned long	x = 0;

	if (i == 0) return 0;
	if (i == 1) return 1;
	f1 = 0;
	f2 = 1;
	while (--i) {
		x = f1 + f2;
		f1 = f2;
		f2 = x;
	}
	return x;
}

int main (int argc, char *argv[])
{
	unsigned long	x = 0;
	unsigned long	b;
	unsigned	i;

	for (i = 0; i < 10; i++) {
		x = fib(i);
		b = flsl(x);
		printf("%ld %ld\n", x, b);
	}
	return 0;
}
