/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

// The size of the length field can be set by the cache - by how big
// the buffers are - use unsigned - <=2**26, 2 bytes, > 2**16 4 bytes
// could even just encode the length into bytes and put it back together.
// then it could be 3 bytes :-)

#include <stdio.h>
#include <string.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>

#include "buf.h"
#include "dev.h"
#include "ht_disk.h"

typedef struct Hash_s {
	unint	numbuckets;
	Buf_s	**bucket;
} Hash_s;

struct Cache_s {
	Dev_s		*dev;
	int		clock;
	Buf_s		*buf;
	Hash_s		hash;
	CacheStat_s	stat;
};

CacheStat_s cache_stats (Cache_s *cache) { return cache->stat; }

Cache_s *cache_new(char *filename, u64 numbufs, u64 block_size)
{
FN;
	Cache_s	*cache;
	Buf_s	*b;
	Dev_s	*dev;
	u8	*data;
	u64	i;

	dev = dev_create(filename, block_size);
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
	cache->hash.numbuckets = findprime(numbufs);
	cache->hash.bucket = ezalloc(sizeof(Buf_s *) * cache->hash.numbuckets);
	return cache;
}

static inline Buf_s *hash (Cache_s *cache,  Blknum_t blknum)
{
	return (Buf_s *)&cache->hash.bucket[blknum % cache->hash.numbuckets];
}

Buf_s *lookup (Cache_s *cache, Blknum_t blknum)
{
FN;
	Buf_s	*b;
	Buf_s	*prev;

	prev = hash(cache, blknum);
	b = prev->next;
	while (b) {
		if (b->blknum == blknum) {
			++b->inuse;
			++cache->stat.gets;
			++cache->stat.hits;
			return b;
		}
		b = b->next;
	}
	++cache->stat.miss;
	return NULL;
}

void rmv (Cache_s *cache, Blknum_t blknum)
{
FN;
	Buf_s	*b;
	Buf_s	*prev;

	prev = hash(cache, blknum);
	b = prev->next;
	while (b) {
		if (b->blknum == blknum) {
			b->blknum = 0;
			prev->next = b->next;
			b->next = NULL;
			return;
		}
		prev = b;
		b = b->next;
	}
}

void add (Cache_s *cache, Buf_s *b)
{
FN;
	Buf_s	*prev = hash(cache, b->blknum);

	assert(b->blknum);
	assert(b->next == NULL);
	b->next = prev->next;
	prev->next = b;
}

Buf_s *victim (Cache_s *cache, Blknum_t blknum)
{
FN;
	Buf_s	*b;
	int	i;

	for (i = 0; i < cache->stat.numbufs * 2; i++) {
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
			if (b->blknum) rmv(cache, b->blknum);
			b->blknum = blknum;
			if (blknum) add(cache, b);
			return b;
		}
	}
	fatal("Out of cache buffers");
	return NULL;
}

Buf_s *buf_new (Cache_s *cache)
{
FN;
	Buf_s		*b;
	Blknum_t	blknum;

	blknum = dev_blknum(cache->dev);
	b = victim(cache, blknum);
	b->dirty = TRUE;
	b->crc = 0;
	return b;
}

Buf_s *buf_get (Cache_s *cache, Blknum_t blknum)
{
FN;
	Buf_s *b;

	assert(blknum != 0);
	b = lookup(cache, blknum);
	if (!b) {
		b = victim(cache, blknum);
		dev_fill(b->cache->dev, b);
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

Buf_s *buf_scratch (Cache_s *cache)
{
FN;
	Buf_s *b;

	b = victim(cache, 0);
	return b;
}

void buf_put (Buf_s **bp)
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
			//XXX: this can be true. assert(crc != b->crc);
			b->crc = crc;
			dev_flush(b->cache->dev, b);
			b->dirty = FALSE;
		} else {
			if (crc != b->crc) {
				PRd(b->blknum);
			}
			assert(crc == b->crc);
		}
	}
}

void buf_put_dirty (Buf_s **bp)
{
FN;
	buf_dirty(*bp);
	buf_put(bp);
}

void buf_toss (Buf_s **bp)
{
FN;
	Buf_s *b = *bp;
	Cache_s *cache = b->cache;

	*bp = NULL;
	assert(b->inuse > 0);
	++cache->stat.puts;
	--b->inuse;
}

void buf_free (Buf_s **bp)
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
		if (b->blknum) rmv(cache, b->blknum);
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
