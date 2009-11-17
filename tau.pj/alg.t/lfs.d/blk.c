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
#include <log.h>

// 1. Could keep a linked list of buffers being used by each device -- would
//    really rather have a stack backtrace.
// 2. Need a bclose and a bshutdown

enum { DEFAULT_NUM_BUFS = 3 };

struct dev_s {
	int	d_fd;		/* Open file descriptor			*/
	char	*d_name;	/* Name of file or device being cached	*/
	log_s	*d_log;		/* Log associated with device		*/
	bool	d_lazy;		/* Delay rights until victim selected	*/
};

static int	Num_bufs;	/* Number of buffers			*/
static void	*Data;		/* Save pointer for freeing buffers	*/
static u8	*Base;		/* Block aligned base for buffers	*/
static buf_s	*Clock;		/* Clock for victim selection		*/
static buf_s	*Buf;		/* Buffer structures and buckets	*/

static inline buf_s *hash (dev_s *dev, u64 blknum)
{
	return &Buf[((addr)dev + blknum) % Num_bufs];
}

static buf_s *lookup (dev_s *dev, u64 blknum)
{
	buf_s	*buf = hash(dev, blknum);

	for (buf = buf->b_head; buf; buf = buf->b_next) {
		if ((buf->b_dev == dev) && (buf->b_blknum == blknum)) {
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
	buf_s	*head = hash(buf->b_dev, buf->b_blknum);

	buf->b_next  = head->b_head;
	head->b_head = buf;
}

static void unhash (buf_s *buf)
{
	buf_s	*prev = hash(buf->b_dev, buf->b_blknum);

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

static void blog (buf_s *buf)
{
	dev_s	*dev;
	log_s	*log;
	int	rc;

	log = buf->b_dev->d_log;
	if (!log) return;

	dev = log->lg_dev;
	rc = pwrite(dev->d_fd, buf->b_data, BLK_SIZE,
			log->lg_next * BLK_SIZE);
	if (rc == -1) {
		eprintf("blog pwrite of %s at %lld failed:",
			dev->d_name, log->lg_next);
		return;
	}
	++log->lg_next;
	if (!(log->lg_next % 7)) {
		take_chkpt(log);
	}
}

static void bflush (buf_s *buf)
{
	dev_s	*dev;
	int	rc;

	if (buf->b_dirty) {
		dev = buf->b_dev;
if (dev->d_lazy) {
	printf("FLUSH %llx %llx\n", buf->b_blknum, *((u64 *)buf->b_data));
}
		buf->b_dirty = FALSE;
		rc = pwrite(dev->d_fd, buf->b_data, BLK_SIZE,
				buf->b_blknum * BLK_SIZE);
		if (rc == -1) {
			eprintf("flush pwrite of %s at %lld failed:",
				dev->d_name, buf->b_blknum);
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
					buf->b_use, buf->b_blknum);
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
					buf->b_use, buf->b_blknum);
			}
		}
		printf("\n");
	}
}
#else
bool binuse (void)
{
	buf_s	*buf;
	unint	i;

	for (i = 0; i < Num_bufs; i++) {
		for (buf = Buf[i].b_head; buf; buf = buf->b_next) {
			if (buf->b_use > 0) {
				printf("inuse %d:%ld:%llx\n", (int)(buf - Buf),
					buf->b_use, buf->b_blknum);
				exit(2);
				return TRUE;
			}
		}
	}
	return FALSE;
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

dev_s *bopen (const char *name, log_s *log)
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
	dev->d_log  = log;
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
printf("PUT %llx %llx\n", buf->b_blknum, *((u64 *)buf->b_data));
	assert(buf->b_use > 0);
	if (buf->b_dev->d_lazy) {
		blog(buf);
	} else {
		bflush(buf);
	}
	--buf->b_use;
}

void bdirty (buf_s *buf)
{
	buf->b_dirty = TRUE;
}

buf_s *bget (dev_s *dev, u64 blknum)
{
	buf_s	*buf;
	int	rc;

	if (!blknum) return NULL;

	buf = lookup(dev, blknum);
	if (buf) {
printf("LOOKUP %llx %llx\n", buf->b_blknum, *((u64 *)buf->b_data));
		return buf;
	}
	buf = victim();
	buf->b_dev = dev;
	buf->b_blknum = blknum;
	add(buf);

	rc = pread(dev->d_fd, buf->b_data, BLK_SIZE, blknum * BLK_SIZE);
PRd(rc);
	if (rc == -1) {
		eprintf("bget pread of %s at %lld failed:", dev->d_name, blknum);
		return NULL;
	}
	if (rc != BLK_SIZE) {
		unhash(buf);
		--buf->b_use;
		//memset(((u8 *)buf->b_data) + rc, 0, BLK_SIZE - rc);
		return NULL;
	}
	buf->b_dirty = FALSE;
printf("READ %llx %llx\n", buf->b_blknum, *((u64 *)buf->b_data));
	return buf;
}

buf_s *bnew (dev_s *dev, u64 blknum)
{
	buf_s	*buf;

	if (!blknum) return NULL;

	buf = lookup(dev, blknum);
	if (!buf) {
		buf = victim();
		buf->b_dev = dev;
		buf->b_blknum = blknum;
		add(buf);
	}
	memset(buf->b_data, 0, BLK_SIZE);
	buf->b_dirty = TRUE;
	return buf;
}

void bmorph (buf_s *buf, dev_s *dev, u64 blknum)
{
	buf_s	*old;

	old = lookup(dev, blknum);
	if (old) {
		unhash(old);
		assert(old->b_use == 1);
		old->b_use = 0;
	}
	unhash(buf);
	buf->b_dev    = dev;
	buf->b_blknum = blknum;
	buf->b_dirty  = TRUE;
	add(buf);
	--buf->b_use;
}

void bclear (void)
{
	unint	i;
	void	*save;

	if (binuse()) return;
	for (i = 0; i < Num_bufs; i++) {
		save = Buf[i].b_data;
		zero(Buf[i]);
		Buf[i].b_data = save;
	}
}

void bsync (dev_s *dev)
{
	buf_s	*buf;
FN;
	for (buf = Buf; buf < &Buf[Num_bufs]; buf++) {
		if ((buf->b_dev == dev) && buf->b_dirty) {
			bflush(buf);
		}
	}
}

log_s *bgetlog (dev_s *dev)
{
	return dev->d_log;
}
