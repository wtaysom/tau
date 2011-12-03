/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>

#include "buf.h"
#include "dev.h"
#include "ht_disk.h"

typedef struct Superblock_s {
	u64	magic;
	Bl

Dev_s *dev_create(char *name, u64 block_size)
{
FN;
	Dev_s *dev;
	int fd;

	fd = open(name, O_RDWR | O_CREAT, 0666);
	if (fd == -1) {
		fatal("open %s:", name);
	}
	dev = ezalloc(sizeof(*dev));
	dev->fd = fd;
	dev->next = 1;
	dev->block_size = block_size;
	dev->name = strdup(name);
	return dev;
}

Dev_s *dev_open (char *name, u64 block_size)
{
FN;
	Dev_s *dev;
	int fd;

	fd = open(name, O_RDWR);
	if (fd == -1) {
		fatal("open %s:", name);
	}
	dev = ezalloc(sizeof(*dev));
	dev->fd = fd;
	dev->next = 1;
	dev->block_size = block_size;
	dev->name = strdup(name);
	return dev;
}

Blknum_t dev_blknum (Dev_s *dev)
{
FN;
	return dev->next++;
}

void dev_flush (Dev_s *dev, Buf_s *b)
{
//FN;
	int rc = pwrite(dev->fd, b->d, dev->block_size,
			b->blknum * dev->block_size);
	if (rc == -1) {
		fatal("pwrite of %s at %lld:", dev->name, b->blknum);
	}
}

void dev_fill (Dev_s *dev, Buf_s *b)
{
FN;
	int rc = pread(dev->fd, b->d, dev->block_size,
			b->blknum * dev->block_size);

	if (rc == -1) {
		fatal("pread of %s at %lld:", dev->name, b->blknum);
	}
	b->crc = crc64(b->d, dev->block_size);
}
