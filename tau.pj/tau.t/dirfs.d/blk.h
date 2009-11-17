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

enum {
	BLK_SHIFT = 10 /*12*/,
	BLK_SIZE = 1 << BLK_SHIFT
};

typedef unsigned long long	blkno_t;
typedef struct device_s		device_s;

device_s *init_blk (char *device);

blkno_t bblkno (void *data);
void *bget  (device_s *dev, blkno_t blkno);
void *bnew  (device_s *dev, blkno_t blkno);
void brel   (void *data);
void bdirty (void *data);
void bflush (void *data);
void baudit (char *msg);
void dump_inuse (void);

#endif
