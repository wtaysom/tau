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
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "q.h"
#include "eprintf.h"
#include "debug.h"
#include "blk.h"

struct device_s {
	char	*d_name;
	int	d_fd;
};

typedef struct cache_s	cache_s;
struct cache_s {
	cache_s		*c_next;
	device_s	*c_dev;
	blkno_t		c_blkno;
	dqlink_t	c_link;
	unsigned	c_dirty;
	unsigned	c_inuse;
	char		c_data[BLK_SIZE];
};

enum {	NUM_BUFS = 17 };

static bool	Cache_inited;
static d_q	Lruq;
static d_q	Inuseq;
static cache_s	*Buckets[NUM_BUFS];
static cache_s	Cache[NUM_BUFS];

static void init_cache (void)
{
	cache_s	*cache;

	if (Cache_inited) return;
	Cache_inited = TRUE;

	init_dq( &Lruq);
	init_dq( &Inuseq);
	for (cache = Cache; cache < &Cache[NUM_BUFS]; cache++) {
		enq_dq( &Lruq, cache, c_link);
	}
}

static bool cnt_inuseq (void *link, int *cnt)
{
	cache_s	*cache = container(link, cache_s, c_link);

	++(*cnt);
	if (cache->c_inuse == 0) {
		printf("Inuseq error %p\n", cache);
	}
	return TRUE;
}

static bool cnt_lruq (void *link, int *cnt)
{
	cache_s	*cache = container(link, cache_s, c_link);

	++(*cnt);
	if (cache->c_inuse != 0) {
		printf("Lruq error %p\n", cache);
	}
	return TRUE;
}

static void audit_cache (void)
{
	int	cnt = 0;
	int	inuse;

	foreach_dq( &Inuseq, (qfunc)cnt_inuseq, &cnt);
	inuse = cnt;
	foreach_dq( &Lruq, (qfunc)cnt_lruq, &cnt);

	if (cnt != NUM_BUFS) {
		printf("Bad cnt: %d != %d\n", cnt, NUM_BUFS);
	}
	printf("cnt = %d inuse = %d\n", cnt, inuse);
}

static inline cache_s **hash (blkno_t blkno)
{
	unsigned	h = blkno;

	return &Buckets[h % NUM_BUFS];
}

static void add (cache_s *cache)
{
	cache_s	**bucket;

	bucket = hash(cache->c_blkno);
	cache->c_next = *bucket;
	*bucket = cache;
}

static cache_s *lookup (device_s *dev, blkno_t blkno)
{
	cache_s	**bucket;
	cache_s	*cache;
	cache_s	*prev;

	bucket = hash(blkno);
	cache  = *bucket;
	if (!cache) return 0;
	if ((cache->c_blkno == blkno) && (cache->c_dev == dev)) {
		return cache;
	}
	for (;;) {
		prev  = cache;
		cache = cache->c_next;
		if (!cache) return 0;
		if ((cache->c_blkno == blkno) && (cache->c_dev == dev)) {
			prev->c_next  = cache->c_next;
			cache->c_next = *bucket;
			*bucket       = cache;
			return cache;
		}
	}
}

static void drop (blkno_t blkno)
{
	cache_s	**bucket;
	cache_s	*cache;
	cache_s	*prev;

	bucket = hash(blkno);
	cache  = *bucket;
	if (!cache) return;
	if (cache->c_blkno == blkno) {
		*bucket = cache->c_next;
		return;
	}
	for (;;) {
		prev  = cache;
		cache = cache->c_next;
		if (!cache) return;
		if (cache->c_blkno == blkno) {
			prev->c_next = cache->c_next;
			return;
		}
	}
}

static cache_s *victim (void)
{
	cache_s	*cache;

	deq_dq( &Lruq, cache, c_link);
	assert(cache);
	drop(cache->c_blkno);
	cache->c_dirty = FALSE;

	return cache;
}

static void read_blk (cache_s *cache)
{
	device_s	*dev = cache->c_dev;
	int		rc;
FN;
	rc = pread(dev->d_fd, cache->c_data, sizeof(cache->c_data),
			cache->c_blkno * sizeof(cache->c_data));
	if (rc == -1) {
		eprintf("read_blk %s blkno=%llx:",
			dev->d_name, cache->c_blkno);
	}
//	if (rc < sizeof(cache->c_data)) {
//		eprintf("Only read %d bytes at blkno=%llx", rc, cache->c_blkno);
//	}
}

static void write_blk (cache_s *cache)
{
	device_s	*dev = cache->c_dev;
	int		rc;
FN;
	rc = pwrite(dev->d_fd, cache->c_data, sizeof(cache->c_data),
			cache->c_blkno * sizeof(cache->c_data));
	if (rc == -1) {
		eprintf("write_blk %s blkno=%llx:",
			dev->d_name, cache->c_blkno);
	}
	if (rc < sizeof(cache->c_data)) {
		eprintf("Only wrote %d bytes at %s blkno=%llx",
			rc, dev->d_name, cache->c_blkno);
	}
	cache->c_dirty = FALSE;
}

blkno_t bblkno (void *data)
{
	cache_s	*cache = container(data, cache_s, c_data);

	if (!data) return 0;
	return cache->c_blkno;
}

void *bget (device_s *dev, blkno_t blkno)
{
	cache_s	*cache;
FN;
	if (!blkno) return NULL;

	cache = lookup(dev, blkno);
	if (!cache) {
		cache = victim();
		cache->c_blkno = blkno;
		cache->c_dev   = dev;
		read_blk(cache);
		add(cache);
	}
	rmv_dq( &cache->c_link);
	enq_dq( &Inuseq, cache, c_link);
	++cache->c_inuse;
	return cache->c_data;
}

void *bnew (device_s *dev, blkno_t blkno)
{
	cache_s	*cache;
FN;
	if (!blkno) return NULL;

	cache = lookup(dev, blkno);
PRp(cache);
	if (!cache) {
		cache = victim();
PRp(cache);
		cache->c_blkno = blkno;
		cache->c_dev   = dev;
		add(cache);
	}
	zero(cache->c_data);
	cache->c_dirty = TRUE;
	rmv_dq( &cache->c_link);
	enq_dq( &Lruq, cache, c_link);
	++cache->c_inuse;
	return cache->c_data;

}

void brel (void *data)
{
	cache_s	*cache = container(data, cache_s, c_data);

//FN;
	if (!data) return;
	assert(cache->c_inuse);
	if (--cache->c_inuse) return;
	if (cache->c_dirty) write_blk(cache);

	rmv_dq( &cache->c_link);
	enq_dq( &Lruq, cache, c_link);
}

void bflush (void *data)
{
	cache_s	*cache = container(data, cache_s, c_data);

FN;
	if (!data) return;
	assert(cache->c_inuse);
	cache->c_dirty = TRUE;
	write_blk(cache);
}

void bdirty (void *data)
{
	cache_s	*cache = container(data, cache_s, c_data);

	if (!data) return;
	assert(cache->c_inuse);
	cache->c_dirty = TRUE;
}

void baudit (char *msg)
{
	printf("baudit: %s ", msg);
	audit_cache();
}

void dump_inuse ()
{
	cache_s	*cache;

	for (cache = Cache; cache < &Cache[NUM_BUFS]; cache++) {
		if (cache->c_inuse) {
			printf("device=%s blkno=%llu dirty=%c inuse=%d ",
				cache->c_dev ? cache->c_dev->d_name : "none",
				cache->c_blkno, (cache->c_dirty ? 'T':'F'),
				cache->c_inuse);
			printf("type=%u\n", ((unsigned short *)cache->c_data)[0]);
		}
	}
}

device_s *init_blk (char *device)
{
	device_s	*dev;
	struct stat	sb;
	int		rc;
FN;
	init_cache();
	dev = ezalloc(sizeof(*dev));
	dev->d_name = device;
	dev->d_fd = open(device, O_RDWR | O_CREAT, 0666);
	if (dev->d_fd == -1) {
		eprintf("Couldn't open %s:", device);
	}
	rc = fstat(dev->d_fd, &sb);

	return dev;
}
