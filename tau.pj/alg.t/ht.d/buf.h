/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BUF_H_
#define _BUF_H_ 1

#include <style.h>

#include "crfs.h"

struct Buf_s {
	Buf_s	*next;
	Inode_s	*inode;
	u64	blknum;
	u64	crc;
	int	inuse;
	bool	dirty;
	bool	clock;
	void	*user;
	void	*d;
};

typedef struct CacheStat_s {
	int	numbufs;
	s64	gets;
	s64	puts;
	s64	hits;
	s64	miss;
} CacheStat_s;

#define PRcache()	cache_trace(__FUNCTION__, __LINE__)

static inline void buf_dirty (Buf_s *b) { b->dirty = TRUE; }

CacheStat_s cache_stats(void);
void cache_start(u64 numbufs);
bool cache_balanced(void);
void cache_pr(void);
void cache_invalidate(void);
void cache_trace(const char *fn, int line);

Buf_s *buf_new(Inode_s *inode, Blknum_t blknum);
Buf_s *buf_get(Inode_s *inode, Blknum_t blknum);
Buf_s *buf_scratch(void);

void buf_free     (Buf_s **bp);
void buf_put      (Buf_s **bp);
void buf_put_dirty(Buf_s **bp);
void buf_toss     (Buf_s **bp);

void buf_pin      (Buf_s *b);
void buf_unpin    (Buf_s *b);

#endif
