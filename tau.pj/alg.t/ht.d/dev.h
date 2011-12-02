/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _DEV_H_
#define _DEV_H_ 1

#include "ht_disk.h"

typedef struct Dev_s {
	int	fd;
	u64	next; // Next block num to allocate
	u64	block_size;
	char	*name;
} Dev_s;


Dev_s *dev_create(char *name, u64 block_size);
Dev_s *dev_open(char *name, u64 block_size);
Blknum_t dev_blknum (Dev_s *dev);
void dev_flush(Dev_s *dev, Buf_s *b);
void dev_fill(Dev_s *dev, Buf_s *b);

#endif
