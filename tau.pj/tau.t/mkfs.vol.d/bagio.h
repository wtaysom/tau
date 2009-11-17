/****************************************************************************
 |  (C) Copyright 2009 Novell, Inc. All Rights Reserved.
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

#ifndef _BAGIO_H_
#define _BAGIO_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

enum { BLK_SHIFT = 12, BLK_SIZE = (1<<BLK_SHIFT), BLK_MASK = BLK_SIZE - 1 };

void  binit    (int num_bufs);
void  bdump    (void);
void  binuse   (void);
void *bget     (ki_t key, u64 blkno);
void *bnew     (ki_t key, u64 blkno);
void  bput     (void *b);
void  bdirty   (void *b);

#endif
