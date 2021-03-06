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
#include "crfs.h"
#include "dev.h"
#include "ht_disk.h"

typedef struct Hash_s {
	unint	numbuckets;
	Buf_s	**bucket;
} Hash_s;

struct Cache_s {
	int		clock;
	Buf_s		*buf;
	Hash_s		hash;
	CacheStat_s	stat;
};

static Cache_s	Cache;

CacheStat_s cache_stats (void) { return Cache.stat; }

void cache_start (u64 numbufs)
{
FN;
	Buf_s	*b;
	u8	*data;
	u64	i;

	b = ezalloc(sizeof(*b) * numbufs);
	data = ezalloc(numbufs * BLOCK_SIZE);
	Cache.buf = b;
	Cache.stat.numbufs = numbufs;
	for (i = 0; i < numbufs; i++) {
		b[i].d = &data[i * BLOCK_SIZE];
	}
	Cache.hash.numbuckets = findprime(numbufs);
	Cache.hash.bucket = ezalloc(sizeof(Buf_s *) * Cache.hash.numbuckets);
}

static inline Buf_s *hash (Inode_s *inode, Blknum_t blknum)
{
	return (Buf_s *)&Cache.hash.bucket[((addr)inode + blknum)
						% Cache.hash.numbuckets];
}

static Buf_s *lookup (Inode_s *inode, Blknum_t blknum)
{
FN;
	Buf_s	*b;
	Buf_s	*prev;

	prev = hash(inode, blknum);
	b = prev->next;
	while (b) {
		if ((b->blknum == blknum) && (b->inode == inode)){
			++b->inuse;
			++Cache.stat.gets;
			++Cache.stat.hits;
			return b;
		}
		b = b->next;
	}
	++Cache.stat.miss;
	return NULL;
}

static void rmv (Inode_s *inode, Blknum_t blknum)
{
FN;
	Buf_s	*b;
	Buf_s	*prev;

	prev = hash(inode, blknum);
	b = prev->next;
	while (b) {
		if ((b->blknum == blknum) && (b->inode == inode)){
			b->blknum = 0;
			prev->next = b->next;
			b->next = NULL;
			return;
		}
		prev = b;
		b = b->next;
	}
}

static void add (Buf_s *b)
{
FN;
	Buf_s	*prev = hash(b->inode, b->blknum);

	assert(b->blknum);
	assert(b->next == NULL);
	b->next = prev->next;
	prev->next = b;
}

static Buf_s *victim (void)
{
FN;
	Buf_s	*b;
	int	i;

	for (i = 0; i < Cache.stat.numbufs * 2; i++) {
		++Cache.clock;
		if (Cache.stat.numbufs <= Cache.clock) {
			Cache.clock = 0;
		}
		if (Cache.buf[Cache.clock].clock) {
			Cache.buf[Cache.clock].clock = FALSE;
			continue;
		}
		b = &Cache.buf[Cache.clock];
		if (!b->inuse) {
			memset(b->d, 0, BLOCK_SIZE);
			++b->inuse;
			++Cache.stat.gets;
			if (b->blknum) rmv(b->inode, b->blknum);
			b->inode = NULL;
			b->blknum = 0;
			b->crc = 0;
			return b;
		}
	}
	fatal("Out of cache buffers");
	return NULL;
}

Buf_s *buf_new (Inode_s *inode, Blknum_t blknum)
{
FN;
	Buf_s		*b;

	b = victim();
	b->inode = inode;
	b->blknum = blknum;
	if (blknum) add(b);
	return b;
}

Buf_s *buf_get (Inode_s *inode, Blknum_t blknum)
{
FN;
	Buf_s *b;

	assert(blknum != 0);
	b = lookup(inode, blknum);
	if (!b) {
		b = buf_new(inode, blknum);
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

Buf_s *buf_scratch (void)
{
FN;
	Buf_s *b;

	b = victim();
	return b;
}

void buf_put (Buf_s **bp)
{
FN;
	Buf_s *b = *bp;

	*bp = NULL;
	assert(b->blknum == ((Node_s *)b->d)->blknum);
	assert(0 < b->inuse);
	++Cache.stat.puts;
	--b->inuse;
	if (!b->inuse) {
		u64 crc = crc64(b->d, BLOCK_SIZE);
		if (b->dirty) {
			if (crc == b->crc) {
				printf("Didn't change %lld\n", b->blknum);
			}
			//XXX: this can be true. assert(crc != b->crc);
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

	*bp = NULL;
	assert(0 < b->inuse);
	++Cache.stat.puts;
	--b->inuse;
}

void buf_free (Buf_s **bp)
{
FN;
	Buf_s *b = *bp;

	*bp = NULL;
	assert(b->blknum == ((Node_s *)b->d)->blknum);
	assert(0 < b->inuse);
	++Cache.stat.puts;
	--b->inuse;
	if (!b->inuse) {
		if (b->blknum) rmv(b->inode, b->blknum);
	}
	//XXX: Don't know what to do yet with freed block
}

void cache_trace (const char *fn, int line)
{
	if (Cache.stat.gets == Cache.stat.puts) {
		printf("%s<%d> == %lld\n", fn, line, Cache.stat.gets);
	} else if (Cache.stat.puts < Cache.stat.gets) {
		printf("%s<%d> %lld puts < %lld gets\n",
			fn, line, Cache.stat.puts, Cache.stat.gets);
	} else {
		printf("BAD %s<%d> %lld gets < %lld puts\n",
			fn, line, Cache.stat.gets, Cache.stat.puts);
	}
}

void cache_invalidate (void)
{
FN;
	int i;

	for (i = 0; i < Cache.stat.numbufs; i++) {
		Cache.buf[i].blknum = 0;
	}
}


bool cache_balanced (void)
{
FN;
	if (Cache.stat.gets != Cache.stat.puts) {
		fatal("gets %lld != %lld puts %d",
			Cache.stat.gets, Cache.stat.puts,
			Cache.stat.gets - Cache.stat.puts);
		return FALSE;
	}
// printf("balanced gets=%lld puts=%lld\n",
//        Cache.stat.gets, Cache.stat.puts);
// stacktrace();
//  cache_invalidate(cache);
	return TRUE;
}

void buf_pin (Buf_s *b)
{
	++Cache.stat.puts;
}

void buf_unpin (Buf_s *b)
{
	++Cache.stat.gets;
}

