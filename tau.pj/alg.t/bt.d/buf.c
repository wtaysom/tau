/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

// The size of the length field can be set by the cache - by how big
// the buffers are - use unsigned - <=2**26, 2 bytes, > 2**16 4 bytes
// could even just encode the length into bytes and put it back together.
// then it could be 3 bytes :-)

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "buf.h"

typedef struct Dev_s {
	int fd;
	u64 next;	// Next block num to allocate
	u64 block_size;
	char *name;
} Dev_s;

struct Cache_s {
	Dev_s *dev;
	int clock;
	int numbufs;
	Buf_s *buf;
};


Dev_s *dev_open(char *name, u64 block_size) {
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

void dev_block(Dev_s *dev, Buf_s *b) {
	b->block = dev->next++;
	memset(b->d, 0, dev->block_size);
}

static void dev_flush(Buf_s *b)
{
	if (b->dirty) {
		Dev_s *dev = b->cache->dev;
		int rc = pwrite(dev->fd, b->d, dev->block_size,
				b->block * dev->block_size);
		if (rc == -1) {
			fatal("pwrite of %s at %lld:", dev->name, b->block);
		}
	}
}

static void dev_fill(Buf_s *b)
{
	Dev_s *dev = b->cache->dev;
	int rc = pread(dev->fd, b->d, dev->block_size, b->block * dev->block_size);

	if (rc == -1) {
		fatal("pread of %s at %lld:", dev->name, b->block);
	}
}

Cache_s *cache_new(char *filename, u64 numbufs, u64 block_size) {
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
	cache->numbufs = numbufs;
	for (i = 0; i < numbufs; i++) {
		b[i].cache = cache;
		b[i].d = &data[i * block_size];
	}
	return cache;
}

// TODO(taysom) Implement a hash lookup
Buf_s *lookup(Cache_s *cache, u64 block) {
	int i;

	for (i = 0; i < cache->numbufs; i++) {
		if (cache->buf[i].block == block) {
			++cache->buf[i].inuse;
			return &cache->buf[i];
		}
	}
	return NULL;
}

static Buf_s *victim(Cache_s *cache)
{
	Buf_s *b;

	for (;;) {
		++cache->clock;
		if (cache->clock >= cache->numbufs) {
			cache->clock = 0;
		}
		b = &cache->buf[cache->clock];
		if (!b->inuse) return b;
		// This is not a clock alg yet
	}
}

Buf_s *buf_new(Cache_s *cache)
{
	Buf_s *b;

	b = victim(cache);
	dev_block(cache->dev, b);
	++b->inuse;
	b->dirty = TRUE;
	return b;
}

Buf_s *buf_get(Cache_s *cache, u64 block)
{
	Buf_s *b;

	b = lookup(cache, block);
	if (b) return b;
	b = victim(cache);
	if (!b) return NULL;
	++b->inuse;
	b->block = block;
	dev_fill(b);
	return b;
}

Buf_s *buf_scratch(Cache_s *cache)
{
	Buf_s *b;
	
	b = victim(cache);
	++b->inuse;
	return b;
}

void buf_put(Buf_s *b)
{
	dev_flush(b);
	--b->inuse;
}




#if 0

func (dev *Dev) Fill(b *Buf) {
	_, e := dev.file.ReadAt(b.Data, b.Blkno * dev.blockSize)
	if e != nil {
		if e.String() != "EOF" {
			fmt.Fprintln(os.Stderr, "Read failed", e)
		}
	}
}

func (dev *Dev) Flush(b *Buf) {
	_, e := dev.file.WriteAt(b.Data, b.Blkno * dev.blockSize)
	if e != nil {
		fmt.Fprintln(os.Stderr, "Write failed", e)
	}
}

func (cache *Cache) readRoot() {
	type rootptr *Root

	b := cache.GetBuf(RootBlkno)
	p := unsafe.Pointer(&b.Data)
	r := rootptr(p)
fmt.Println("Root=", r, *r)
}

func NewCache(filename string, numbufs, blockSize int64) *Cache {
	dev := OpenDev(filename, blockSize)
	if dev == nil { return nil }
	bufs := make([]Buf, numbufs)
	data := make([]byte, numbufs * blockSize)
	cache := &Cache{bufs, 0, dev}
	for i := int64(0); i < int64(len(cache.Buf)); i++ {
		cache.Buf[i].Cache = cache
		cache.Buf[i].Data = data[i * blockSize: (i+1)*blockSize]
	}
	return cache
}

func (cache *Cache) Clear() {
	for i := 0; i < len(cache.Buf); i++ {
		cache.Buf[i].Blkno = -1
		cache.Buf[i].inuse = 0;
	}
}

func (cache *Cache) Audit() bool {
	if cache.clock >= len(cache.Buf) { return false }
	return true
}

func (cache *Cache) victim() *Buf {
	var b *Buf
	for {
		cache.clock++
		if cache.clock >= len(cache.Buf) {
			cache.clock = 0
		}
		b = &cache.Buf[cache.clock]
		if b.inuse == 0 { return b }
	}
	return b
}

func (cache *Cache) lookup(blkno int64) *Buf {
	for i := 0; i < len(cache.Buf); i++ {
		if blkno == cache.Buf[i].Blkno {
			return &cache.Buf[i]
		}
	}
	return nil
}

func (cache *Cache) NewBuf() *Buf {
	b := cache.victim()
	cache.dev.NewBlock(b)
	b.inuse++
	return b
}

func (cache *Cache) GetBuf(blkno int64) *Buf {
	b := cache.lookup(blkno)
	if b == nil {
		b = cache.victim()
		b.Blkno = blkno
		cache.dev.Fill(b)
	}
	b.inuse++
	return b
}

func (b *Buf) Put() {
	b.Cache.dev.Flush(b)
	b.inuse--
}

func (b *Buf) Swap(a *Buf) {
	t := a.Data
	a.Data = b.Data
	b.Data = t
}

func Scratch(length int64) *Buf {
	b := make([]byte, length)
	return &Buf{b, 0, 0, nil}
}
#endif
