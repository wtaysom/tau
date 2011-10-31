/*
 * Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#include <debug.h>

typedef struct StA_s {
	int a, b;
	char c[];
} StA_s;

typedef struct StB_s {
	int a, b;
	char c[0];
} StB_s;

typedef u32 Key_t;
typedef u32 Block_t;

typedef struct Twig_s {
	Key_t key;
	Block_t block;
} Twig_s;

typedef struct Head_s {
	u8	kind;
	u8	unused1;
	u16	num_recs;
	Block_t	block;
	union {
		struct {
			Block_t	frist;
			Twig_s	twig[0];
		};
		struct {
			u16	free;
			u16	end;
			u16	rec[0];
		};
	};
} Head_s;

enum {	SZ_A_ST = sizeof(StA_s),
	SZ_B_ST = sizeof(StB_s) };

int main(int argc, char *argv[]) {
	PRd(SZ_A_ST);
	PRd(SZ_B_ST);
	PRd(sizeof(Twig_s));
	PRd(sizeof(Head_s));
	return 0;
}
