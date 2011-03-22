// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"fmt"
	"runtime"
	"flag"
	"os"
	"unsafe"
)

func test(e bool) {
	if (e) { return }
	_, file, line, _ := runtime.Caller(1);
	fmt.Printf("Error at %s<%d>\n", file, line)
}

const BufSize = 256
const RootBlkno = 1
const RootMagic = 0x123456789abcdef

type Root struct {
	magic	int64
	nextblk	int64
}

type Buf struct {
	data	[BufSize]byte
	blkno	int64
	inuse	int
	cache	*Cache
}

type Dev struct {
	file	*os.File
	next	int64		// Next block num to allocate
}

type Cache struct {
	buf	[]Buf
	clock	int
	dev	*Dev
}

func OpenDev(name string) *Dev {
	f, e := os.Open(name, os.O_RDWR | os.O_CREAT, 0666);
	if e != nil {
		fmt.Fprintln(os.Stderr, "Failed to open", name, e)
		return nil
	}
	return &Dev{f, 1}
}

func (dev *Dev) NewBlock(b *Buf) {
	b.blkno = dev.next; dev.next++
	for i := 0; i < len(b.data); i++ {
		b.data[i] = 0
	}
}

func (dev *Dev) Fill(b *Buf) {
	_, e := dev.file.ReadAt(b.data[:], b.blkno * BufSize)
	if e != nil {
		if e.String() != "EOF" {
			fmt.Fprintln(os.Stderr, "Read failed", e)
		}
	}
}

func (dev *Dev) Flush(b *Buf) {
	_, e := dev.file.WriteAt(b.data[:], b.blkno * BufSize)
	if e != nil {
		fmt.Fprintln(os.Stderr, "Write failed", e)
	}
}

func (cache *Cache) readRoot() {
//	b := make([]byte, 256)
//	c := &b
	type rootptr *Root

	b := cache.GetBuf(RootBlkno)
	p := unsafe.Pointer(&b.data)
	r := rootptr(p)
fmt.Println("Root=", r, *r)
}

func NewCache(num_bufs int, dev *Dev) *Cache {
	b := make([]Buf, num_bufs)
	cache := &Cache{b, 0, dev}
	for i := 0; i < len(cache.buf); i++ {
		cache.buf[i].cache = cache
	}
	return cache
}

func (cache *Cache) Clear() {
	for i := 0; i < len(cache.buf); i++ {
		cache.buf[i].blkno = -1
		cache.buf[i].inuse = 0;
	}
}

func (cache *Cache) Audit() bool {
	if cache.clock >= len(cache.buf) { return false }
	return true
}

func (cache *Cache) victim() *Buf {
	var b *Buf
	for {
		cache.clock++
		if cache.clock >= len(cache.buf) {
			cache.clock = 0
		}
		b = &cache.buf[cache.clock]
		if b.inuse == 0 { return b }
	}
	return b
}

func (cache *Cache) lookup(blkno int64) *Buf {
	for i := 0; i < len(cache.buf); i++ {
		if blkno == cache.buf[i].blkno {
			return &cache.buf[i]
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
		b.blkno = blkno
		cache.dev.Fill(b)
	}
	b.inuse++
	return b
}

func (b *Buf) Put() {
	b.cache.dev.Flush(b)
	b.inuse--
}

func main() {
	flag.Parse()
	cachefile := "/tmp/cache"
	if flag.NArg() > 0 {
		cachefile = flag.Arg(0);
	}
	fmt.Printf("cache file = %s\n", cachefile);
	d := OpenDev(cachefile)
	if d == nil {
		fmt.Fprintln(os.Stderr, "open failed", cachefile);
	}
	cache := NewCache(10, d)

// Test 1. Get a buffer, set a value, and Put it back
	b := cache.NewBuf()
	test(b != nil)
	test(b.blkno == 1)
	b.data[0] = 'A'
fmt.Printf("b=%p\n", b)
	b.Put()
	
	cache.Clear()

// Test 2. Get the buffer from test 1 and check it has what we set
// This is not a good test, because it depends on test 1.
// This test did fail to detect that we read the block into a
// separate buffer. Now we have to copies of the buffer in memory.
	b = cache.GetBuf(1)
fmt.Printf("b=%p\n", b)
fmt.Println("b=", b)
	test(b != nil)
	test(b.blkno == 1)
	test(b.data[0] == 'A')
	b.Put()
	test(cache.Audit())

// Test 3. Use up all the buffers without releasing any of them
// Can't really test for running off of end nor can we test for
// infinite loops.
	for i := 0; i < len(cache.buf); i++ {
		b = cache.GetBuf(int64(i))
		b.Put()
	}
	b = cache.GetBuf(int64(len(cache.buf)));
	test(b != nil);

	test(cache.Audit());
}
