/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef __APPLE__
#define _XOPEN_SOURCE 500
#endif

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

Dev_s *dev_create(char *name)
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
	dev->name = strdup(name);
	return dev;
}

Dev_s *dev_open (char *name)
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
	dev->name = strdup(name);
	return dev;
}

void dev_flush (Buf_s *b)
{
//FN;
	Dev_s	*dev = b->crnode->volume->dev;

	int rc = pwrite(dev->fd, b->d, BLOCK_SIZE,
			b->blknum * BLOCK_SIZE);
	if (rc == -1) {
		fatal("pwrite of %s at %lld:", dev->name, b->blknum);
	}
}

void dev_fill (Buf_s *b)
{
FN;
	Dev_s	*dev = b->crnode->volume->dev;

	int rc = pread(dev->fd, b->d, BLOCK_SIZE,
			b->blknum * BLOCK_SIZE);

	if (rc == -1) {
		fatal("pread of %s at %lld:", dev->name, b->blknum);
	}
	b->crc = crc64(b->d, BLOCK_SIZE);
}
