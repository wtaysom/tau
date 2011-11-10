/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <ctype.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

enum {	BYTES_PER_LINE = 16,
	SZ_BUF = 1<<12 };

u64 Buf[SZ_BUF/sizeof(u64)];

void dump (FILE *f)
{
	int	i = 0;
	int	j;
	int	k;
	int	line = 0;
	u8	*b = (u8 *)Buf;
	u8	c;
	size_t	n;

	printf("         0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15\n");
	for (;;) {
		n = fread(Buf, 1, sizeof(Buf), f);
		if (!n) return;
		for (i = 0; i < n; i = k) {
			printf("%6x ", line);
			line += 16;
			k = i;
			for (j = 0; (j < BYTES_PER_LINE) && (k < n); j++, k++) {
				c = b[k];
				printf(" %1x%1x", c & 0xf, (c >> 4) & 0xf);
			}
			for (; j < BYTES_PER_LINE; j++) {
				printf("   ");
			}
			printf(" | ");
			k = i;
			for (j = 0; (j < BYTES_PER_LINE) && (k < n); j++, k++) {
				c = b[k];
				if (isprint(c)) {
					putchar(c);
				} else {
					putchar('.');
				}
			}
			putchar('\n');
		}
	}
}

int main (int argc, char *argv[])
{
	FILE	*f;
	int	i;

	if (argc == 1) {
		dump(stdin);
	} else {
		for (i = 1; i < argc; i++) {
			f = fopen(argv[i], "r");
			if (!f) {
				fatal("couldn't fopen %s:", argv[i]);
			}
			dump(f);
			fclose(f);
		}
	}
	return 0;
}
