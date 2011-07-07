/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <style.h>

#include <crc.h>

enum { BIG_PRIME = 824633720837ULL };

static inline u8 hash (s64 offset)
{
#if 0
	return ((offset * 1103515245 + 12345) /65536) % 32768;
	return offset % 805306457 % 25165843;
	return crc32( &offset, sizeof(s64));
	u64	x = offset;
	x = x ^ (x >> 32);
	x = x ^ (x >> 16);
	x = x ^ (x >> 8);
	return x;
#endif
	return crc32( &offset, sizeof(s64));
}

int main (int argc, char *argv[]) {
	s64	x = 0;
	s64	y = (1LL << 32) + 4096;
	u8	a, b;
	int	i;

	for (i = 0; i < 1000; i++) {
		a = hash(x);
		b = hash(y);
		printf("%2x %2x\n", a, b);
		++x;
		++y;
	}
	return 0;
}
