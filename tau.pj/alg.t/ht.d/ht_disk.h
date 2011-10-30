/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#ifndef _HT_DISK_H_
#define _HT_DISK_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

enum { LEAF = 1, BRANCH };

#define TUESDAY 0
#define WEDNESDAY 1
#define THUSDAY 0

#if TUESDAY
typedef u64 Key_t;
typedef u64 Block_t;
#elif WEDNESDAY
typedef u32 Key_t;
typedef u64 Block_t;
#elif THURSDAY
typedef u64 Key_t;
typedef u32 Block_t;
#else
typedef u32 Key_t;
typedef u32 Block_t;
#endif

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

#define PACK(dst, data) memmove((dst), &(data), sizeof(data))
#define UNPACK(src, data) memmove(&(data), (src), sizeof(data))

#endif
