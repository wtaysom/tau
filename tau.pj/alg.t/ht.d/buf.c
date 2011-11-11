/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

// The size of the length field can be set by the cache - by how big
// the buffers are - use unsigned - <=2**26, 2 bytes, > 2**16 4 bytes
// could even just encode the length into bytes and put it back together.
// then it could be 3 bytes :-)

#define _XOPEN_SOURCE 500

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "buf.h"
#include "ht_disk.h"

typedef struct Dev_s {
	int fd;
	u64 next; // Next block num to allocate
	u64 block_size;
	char *name;
} Dev_s;

struct Cache_s {
	Dev_s *dev;
	int clock;
	Buf_s *buf;
	CacheStat_s stat;
};

CacheStat_s cache_stats (Cache_s *cache) { return cache->stat; }

Dev_s *dev_open(char *name, u64 block_size)
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

void dev_blknum(Dev_s *dev, Buf_s *b)
{
FN;
	b->blknum = dev->next++;
	memset(b->d, 0, dev->block_size);
}

void dev_flush(Buf_s *b)
{
//FN;
	Dev_s *dev = b->cache->dev;
	int rc = pwrite(dev->fd, b->d, dev->block_size,
			b->blknum * dev->block_size);
	if (rc == -1) {
		fatal("pwrite of %s at %lld:", dev->name, b->blknum);
	}
}

void dev_fill(Buf_s *b)
{
FN;
	Dev_s *dev = b->cache->dev;
	int rc = pread(dev->fd, b->d, dev->block_size,
			b->blknum * dev->block_size);

	if (rc == -1) {
		fatal("pread of %s at %lld:", dev->name, b->blknum);
	}
	b->crc = crc64(b->d, dev->block_size);
}

Cache_s *cache_new(char *filename, u64 numbufs, u64 block_size)
{
FN;
	Cache_s *cache;
	Buf_s *b;
	Dev_s *dev;
	u8 *data;
	u64 i;

	dev = dev_open(filename, block_size);
	if (!dev) return NULL;

	b = ezalloc(sizeof(*b) * numbufs);
	data = ezalloc(numbufs * block_size);
	cache = ezalloc(sizeof(*cache));
	cache->dev = dev;
	cache->buf = b;
	cache->stat.numbufs = numbufs;
	for (i = 0; i < numbufs; i++) {
		b[i].cache = cache;
		b[i].d = &data[i * block_size];
	}
	return cache;
}

// TODO(taysom) Implement a hash lookup
Buf_s *lookup(Cache_s *cache, Blknum_t blknum)
{
FN;
	int i;

	for (i = 0; i < cache->stat.numbufs; i++) {
		if (cache->buf[i].blknum == blknum) {
			++cache->buf[i].inuse;
			++cache->stat.gets;
			++cache->stat.hits;
			return &cache->buf[i];
		}
	}
	++cache->stat.miss;
	return NULL;
}

Buf_s *victim(Cache_s *cache)
{
FN;
	Buf_s *b;

	for (;;) {
		++cache->clock;
		if (cache->clock >= cache->stat.numbufs) {
			cache->clock = 0;
		}
		if (cache->buf[cache->clock].clock) {
			cache->buf[cache->clock].clock = FALSE;
			continue;
		}
		b = &cache->buf[cache->clock];
		if (!b->inuse) {
			memset(b->d, 0, cache->dev->block_size);
			++b->inuse;
			++cache->stat.gets;
			return b;
		}
	}
}

Buf_s *buf_new(Cache_s *cache)
{
FN;
	Buf_s *b;

	b = victim(cache);
	dev_blknum(cache->dev, b);
	b->dirty = TRUE;
	b->crc = 0;
	return b;
}

Buf_s *buf_get(Cache_s *cache, Blknum_t blknum)
{
FN;
	Buf_s *b;

	assert(blknum != 0);
	b = lookup(cache, blknum);
	if (!b) {
		b = victim(cache);
		b->blknum = blknum;
		dev_fill(b);
	}
	b->clock = TRUE;
//b->dirty = TRUE;
	assert(blknum == b->blknum);
if (b->blknum != ((Node_s *)b->d)->blknum) {
	printf("%p %llx %llx != %llx\n",
		b, (u64)blknum,
		(u64)b->blknum, (u64)((Node_s *)b->d)->blknum);
}
	assert(b->blknum == ((Node_s *)b->d)->blknum);
	return b;
}

Buf_s *buf_scratch(Cache_s *cache)
{
FN;
	Buf_s *b;

	b = victim(cache);
	b->blknum = 0;
	return b;
}

void buf_put(Buf_s **bp)
{
FN;
	Buf_s *b = *bp;
	Cache_s *cache = b->cache;

	*bp = NULL;
	assert(b->blknum == ((Node_s *)b->d)->blknum);
	assert(b->inuse > 0);
	++cache->stat.puts;
	--b->inuse;
	if (!b->inuse) {
		u64 crc = crc64(b->d, b->cache->dev->block_size);
		if (b->dirty) {
			if (crc == b->crc) {
				printf("Didn't change %lld\n", b->blknum);
			}
			assert(crc != b->crc);
			b->crc = crc;
			dev_flush(b);
			b->dirty = FALSE;
		} else {
			if (crc != b->crc) {
				PRd(b->blknum);
			}
			assert(crc == b->crc);
		}
	}
}

void buf_put_dirty(Buf_s **bp)
{
FN;
	buf_dirty(*bp);
	buf_put(bp);
}

void buf_toss(Buf_s **bp)
{
FN;
	Buf_s *b = *bp;
	Cache_s *cache = b->cache;

	*bp = NULL;
	assert(b->inuse > 0);
	++cache->stat.puts;
	--b->inuse;
}

void buf_free(Buf_s **bp)
{
FN;
	Buf_s *b = *bp;
	Cache_s *cache = b->cache;

	*bp = NULL;
	assert(b->blknum == ((Node_s *)b->d)->blknum);
	assert(b->inuse > 0);
	++cache->stat.puts;
	--b->inuse;
	if (!b->inuse) {
		b->blknum = 0;
	}
	//XXX: Don't know what to do yet with freed block
}

void cache_invalidate (Cache_s *cache)
{
FN;
	int i;

	for (i = 0; i < cache->stat.numbufs; i++) {
		cache->buf[i].blknum = 0;
	}
}

bool cache_balanced(Cache_s *cache)
{
FN;
	if (cache->stat.gets != cache->stat.puts) {
		fatal("gets %lld != %lld puts %d",
			cache->stat.gets, cache->stat.puts,
			cache->stat.gets - cache->stat.puts);
		return FALSE;
	}
// printf("balanced gets=%lld puts=%lld\n",
//        cache->stat.gets, cache->stat.puts);
// stacktrace();
//  cache_invalidate(cache);
	return TRUE;
}
