/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BUF_H_
#define _BUF_H_ 1

#include <style.h>

#include "crfs.h"

struct Buf_s {
	Buf_s		*next;
	Crnode_s	*crnode;
	u64		blknum;
	u64		crc;
	int		inuse;
	bool		dirty;
	bool		clock;
	void		*user;
	void		*d;
};

typedef struct CacheStat_s {
	int	numbufs;
	s64	gets;
	s64	puts;
	s64	hits;
	s64	miss;
} CacheStat_s;

static inline void buf_dirty (Buf_s *b) { b->dirty = TRUE; }

CacheStat_s cache_stats(void);
void cache_start(u64 numbufs);
bool cache_balanced(void);
void cache_pr(void);

Buf_s *buf_new(Crnode_s *crnode, Blknum_t blknum);
Buf_s *buf_get(Crnode_s *crnode, Blknum_t blknum);
Buf_s *buf_scratch(void);
void buf_free(Buf_s **bp);
void buf_put(Buf_s **bp);
void buf_put_dirty(Buf_s **bp);
void buf_toss(Buf_s **bp);

#endif
