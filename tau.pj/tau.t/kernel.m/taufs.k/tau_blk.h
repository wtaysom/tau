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

#ifndef _TAU_BLK_H_
#define _TAU_BLK_H_ 1

#ifndef _LINUX_FS_H
#include <linux/fs.h>
#endif

#ifndef _STYLE_H_
#include <style.h>
#endif

enum {	BLK_SHIFT = 12,
	BLK_SIZE = (1<<BLK_SHIFT),
	BLK_MASK = BLK_SIZE - 1 };

void  tau_bsync  (struct address_space *mapping);
void *tau_bget   (struct address_space *mapping, u64 blknum);
void *tau_bnew   (struct address_space *mapping, u64 blknum);
void  tau_bput   (void *data);
void  tau_bdirty (void *data);
void  tau_blog   (void *data);
int   tau_bflush (void *data);
u64   tau_bnum   (void *data);

#endif
