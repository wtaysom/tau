/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#ifndef _BLK_H_
#define _BLK_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct log_s	log_s;
typedef struct dev_s	dev_s;
typedef struct buf_s	buf_s;

typedef struct head_s {
	__u64	h_blknum;
	__u32	h_magic;
	__u32	h_reserved[1];
} head_s;

struct buf_s {
	void	*b_data;
	buf_s	*b_head;
	buf_s	*b_next;
	dev_s	*b_dev;
	u64	b_blknum;
	snint	b_use;
	bool	b_dirty;
};

enum { BLK_SHIFT = 8, BLK_SIZE = (1<<BLK_SHIFT), BLK_MASK = BLK_SIZE - 1 };

void   binit    (int num_bufs);
void   bdump    (void);
void   bclear   (void);
bool   binuse   (void);
dev_s *bopen    (const char *name, log_s *log);
void   bclose   (dev_s *dev);
void   blazy    (dev_s *dev);
u64    bdevsize (dev_s *dev);	/* device size in blocks */
buf_s *bget     (dev_s *dev, u64 blknum);
buf_s *bnew     (dev_s *dev, u64 blknum);
void   bput     (buf_s *b);
void   bdirty   (buf_s *b);
log_s *bgetlog  (dev_s *dev);
void   bmorph   (buf_s *b, dev_s *dev, u64 blknum);
void   bsync    (dev_s *dev);

buf_s *alloc_block (dev_s *dev);

#endif
