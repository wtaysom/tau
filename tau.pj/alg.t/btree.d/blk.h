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

typedef struct dev_s	dev_s;

enum {	BLK_SHIFT = 8,
	BLK_SIZE = (1<<BLK_SHIFT),
	BLK_MASK = BLK_SIZE - 1 };

void   binit    (int num_bufs);
void   bdump    (void);
void   binuse   (void);
dev_s *bopen    (const char *name);
void   bsync    (dev_s *dev);
void   bclose   (dev_s *dev);
void   blazy    (dev_s *dev);
u64    bdevsize (dev_s *dev);	/* device size in blocks */
void  *bget     (dev_s *dev, u64 blkno);
void  *bnew     (dev_s *dev, u64 blkno);
void   bput     (void *data);
void   bdirty   (void *data);
u64    bblkno   (void *data);
dev_s *bdev     (void *data);

void  *alloc_block (dev_s *dev);

#endif
