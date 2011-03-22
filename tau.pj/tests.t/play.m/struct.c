/*
 * Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#include <stdio.h>

typedef struct St_s {
	int a, b;
	char c[];
} St_s;

enum { SZ_ST = sizeof(St_s) };

int main(int argc, char *argv[]) {
	printf("ST_ST=%d\n", SZ_ST);
	return 0;
}
