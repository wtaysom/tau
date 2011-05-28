/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#ifndef _BT_DISK_H_
#define _BT_DISK_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

enum { LEAF = 1, BRANCH };

typedef struct Head_s {
	u8	kind;
	u8	is_split;
	u16	num_recs;
	u16	free;
	u16	end;
	u64	block;
	u64	last;
	u16	rec[];
} Head_s;

#define PACK(dst, data) memmove((dst), &(data), sizeof(data))
#define UNPACK(src, data) memmove(&(data), (src), sizeof(data))

#endif
