/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#ifndef _HT_DISK_H_
#define _HT_DISK_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _BUF_H_
#include <buf.h>
#endif

#define TUESDAY 0
#define WEDNESDAY 1
#define THUSDAY 0

#if TUESDAY
typedef u64 Key_t;
typedef u64 Blknum_t;
#elif WEDNESDAY
typedef u32 Key_t;
typedef u64 Blknum_t;
#elif THURSDAY
typedef u64 Key_t;
typedef u32 Blknum_t;
#else
typedef u32 Key_t;
typedef u32 Blknum_t;
#endif

typedef struct Twig_s {
	Key_t		key;
	Blknum_t	blknum;
} Twig_s;

typedef struct Node_s {
	u8		is_leaf;
	u8		unused1;
	u16		numrecs;
	Blknum_t	blknum;
	union {
		struct {
			Blknum_t	first;
			Twig_s		twig[0];
		};
		struct {
			u16	free;
			u16	end;
			u16	rec[0];
		};
	};
} Node_s;

enum {	BLOCK_SHIFT = 8,
	BLOCK_SIZE  = BLOCK_SHIFT,
	MAX_FREE    = (BLOCK_SIZE - sizeof(Node_s)),
	NUM_TWIGS   = MAX_FREE / sizeof(Twig_s),
	TWIGS_LOWER_HALF = NUM_TWIGS / 2,
	TWIGS_UPPER_HALF = NUM_TWIGS - TWIGS_LOWER_HALF,
	LEAF_SPLIT       = MAX_FREE / 3,
	BRANCH_SPLIT     = NUM_TWIGS / 3};

#define PACK(dst, data) memmove((dst), &(data), sizeof(data))
#define UNPACK(src, data) memmove(&(data), (src), sizeof(data))

#endif
