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

#define _XOPEN_SOURCE 500

#include <sys/types.h>
//#define __APPLE__ 1
#ifndef __APPLE__
#include <linux/fs.h>	// BLKGETSIZE64
#endif

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

#include <blk.h>

// 1. Could keep a linked list of buffers being used by each device -- would
//    really rather have a stack backtrace.
// 2. Need a bclose and a bshutdown

enum { DEFAULT_NUM_BUFS = 3 };

struct dev_s {
	int	d_fd;		/* Open file descriptor			*/
	char	*d_name;	/* Name of file or device being cached	*/
	bool	d_lazy;		/* Delay rights until victim selected	*/
};

static int	Num_bufs;	/* Number of buffers			*/
static void	*Data;		/* Save pointer for freeing buffers	*/
static u8	*Base;		/* Block aligned base for buffers	*/
static buf_s	*Clock;		/* Clock for victim selection		*/
static buf_s	*Buf;		/* Buffer structures and buckets	*/

static inline buf_s *hash (dev_s *dev, u64 blkno)
{
	return &Buf[((addr)dev + blkno) % Num_bufs];
}

static buf_s *lookup (dev_s *dev, u64 blkno)
{
	buf_s	*buf = hash(dev, blkno);

	for (buf = buf->b_head; buf; buf = buf->b_next) {
		if ((buf->b_dev == dev) && (buf->b_blkno == blkno)) {
			if (buf->b_use < 0) {
				buf->b_use = 1;
			} else {
				++buf->b_use;
			}
			return buf;
		}
	}
	return NULL;
}

static void add (buf_s *buf)
{
	buf_s	*head = hash(buf->b_dev, buf->b_blkno);

	buf->b_next  = head->b_head;
	head->b_head = buf;
}

static void unhash (buf_s *buf)
{
	buf_s	*prev = hash(buf->b_dev, buf->b_blkno);

	if (!prev->b_head) {
		return;
	}
	if (prev->b_head == buf) {
		prev->b_head = buf->b_next;
		buf->b_next  = NULL;
		return;
	}
	for (prev = prev->b_head; prev; prev = prev->b_next) {
		if (prev->b_next == buf) {
			prev->b_next = buf->b_next;
			buf->b_next  = NULL;
			return;
		}
	}
}

static void bflush (buf_s *buf)
{
	dev_s	*dev;
	int	rc;

	if (buf->b_dirty) {
		dev = buf->b_dev;
		buf->b_dirty = FALSE;
		rc = pwrite(dev->d_fd, buf->b_data, BLK_SIZE,
				buf->b_blkno * BLK_SIZE);
		if (rc == -1) {
			eprintf("bput pwrite of %s at %lld failed:",
				dev->d_name, buf->b_blkno);
			return;
		}
	}
}

static buf_s *victim (void)
{
	buf_s	*buf;
	unint	wrap = 0;

	for (;;) {
		++Clock;
		if (Clock == &Buf[Num_bufs]) {
			Clock = Buf;
			if (++wrap > 2) {
				bdump();
				eprintf("All buffers in use");
				return NULL;
			}
		}
		buf = Clock;
		if (buf->b_use < 0) {
			bflush(buf);
			buf->b_use = 1;
			unhash(buf);
			return buf;
		}
		if (buf->b_use == 0) {
			buf->b_use = -1;
		}
	}
}

void bdump (void)
{
	buf_s	*buf;
	unint	i;

	for (i = 0; i < Num_bufs; i++) {
		printf("%6lu. ", i);
		for (buf = Buf[i].b_head; buf; buf = buf->b_next) {
			if (buf->b_dev) {
				printf("%d:%ld:%llx ", (int)(buf - Buf),
					buf->b_use, buf->b_blkno);
			} else {
				printf("%d:free ", (int)(buf - Buf));
			}
		}
		printf("\n");
	}
}

#if 0
void binuse (void)
{
	buf_s	*buf;
	unint	i;

	for (i = 0; i < Num_bufs; i++) {
		printf("%6lu. ", i);
		for (buf = Buf[i].b_head; buf; buf = buf->b_next) {
			if (buf->b_use > 0) {
				printf("%d:%ld:%llx ", (int)(buf - Buf),
					buf->b_use, buf->b_blkno);
			}
		}
		printf("\n");
	}
}
#else
void binuse (void)
{
	buf_s	*buf;
	unint	i;

	for (i = 0; i < Num_bufs; i++) {
		for (buf = Buf[i].b_head; buf; buf = buf->b_next) {
			if (buf->b_use > 0) {
				printf("inuse %d:%ld:%llx\n", (int)(buf - Buf),
					buf->b_use, buf->b_blkno);
				exit(2);
			}
		}
	}
	//printf("\n");
}
#endif

void binit (int num_bufs)
{
	unint	i;

	if (Buf) {
		fprintf(stderr, "block cache already initialized\n");
		return;
	}
	if (num_bufs == 0) num_bufs = DEFAULT_NUM_BUFS;
	num_bufs |= 1; /* make sure it is at least odd */

	Clock = Buf = ezalloc(sizeof(buf_s) * num_bufs);
	Data = emalloc((num_bufs + 1) * BLK_SIZE);

	Base = (u8 *)(((addr)Data + BLK_MASK) & ~BLK_MASK);
	for (i = 0; i < num_bufs; i++) {
		Buf[i].b_data = &Base[i * BLK_SIZE];
	}
	Num_bufs = num_bufs;
}

dev_s *bopen (const char *name)
{
	dev_s	*dev;

	dev = emalloc(sizeof(*dev));
	dev->d_fd = open(name, O_CREAT|O_RDWR, 0666);
	if (dev->d_fd == -1) {
		free(dev);
		eprintf("bopen couldn't open %s:", name);
		return NULL;
	}
	dev->d_name = strdup(name);
	dev->d_lazy = FALSE;
	return dev;
}

void blazy (dev_s *dev)	/* Helps simulate lost writes */
{
	dev->d_lazy = TRUE;
}

void bclose (dev_s *dev)
{
	close(dev->d_fd);
	free(dev);
	/* Should really go thru all bufs a get rid of ones with dev */
}

u64 bdevsize (dev_s *dev)
{
#ifdef __APPLE__
	fprintf(stderr, "This is an Apple\n");
	return 0;
#else
	u64	size;
	int	rc;

	rc = ioctl(dev->d_fd, BLKGETSIZE64, &size);
	if (rc == -1) {
		return 0;
	}
	return size >> BLK_SHIFT;
#endif
}

void bput (buf_s *buf)
{
	assert(buf->b_use > 0);
	if (!buf->b_dev->d_lazy) {
		bflush(buf);
	}
	--buf->b_use;
}

void bdirty (buf_s *buf)
{
	buf->b_dirty = TRUE;
}

buf_s *bget (dev_s *dev, u64 blkno)
{
	buf_s	*buf;
	int	rc;

	if (!blkno) return NULL;

	buf = lookup(dev, blkno);
	if (buf) {
		return buf;
	}
	buf = victim();
	buf->b_dev = dev;
	buf->b_blkno = blkno;
	add(buf);

	rc = pread(dev->d_fd, buf->b_data, BLK_SIZE, blkno * BLK_SIZE);
	if (rc == -1) {
		eprintf("bget pread of %s at %lld failed:", dev->d_name, blkno);
		return NULL;
	}
	buf->b_dirty = FALSE;
	return buf;
}

buf_s *bnew (dev_s *dev, u64 blkno)
{
	buf_s	*buf;

	//if (!blkno) return NULL;

	buf = lookup(dev, blkno);
	if (!buf) {
		buf = victim();
		buf->b_dev = dev;
		buf->b_blkno = blkno;
		add(buf);
	}
	memset(buf->b_data, 0, BLK_SIZE);
	buf->b_dirty = TRUE;
	return buf;
}
