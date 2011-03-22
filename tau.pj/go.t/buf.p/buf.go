// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

// The size of the length field can be set by the cache - by how big
// the buffers are - use unsigned - <=2**26, 2 bytes, > 2**16 4 bytes
// could even just encode the length into bytes and put it basck together.
// then it could be 3 bytes :-)
package buf

import (
	//"bug"
	"fmt"
	"os"
	"unsafe"
)

const RootBlkno = 1
const RootMagic = 0x123456789abcdef

type Root struct {
	magic	int64
	nextblk	int64
}

type Buf struct {
	Data	[]byte
	Blkno	int64
	inuse	int
	Cache	*Cache
}

type Dev struct {
	file	*os.File
	next	int64		// Next block num to allocate
	blockSize int64
}

type Cache struct {
	Buf	[]Buf
	clock	int
	dev	*Dev
}

func OpenDev(name string, blockSize int64) *Dev {
	f, e := os.Open(name, os.O_RDWR | os.O_CREAT, 0666);
	if e != nil {
		fmt.Fprintln(os.Stderr, "Failed to open", name, e)
		return nil
	}
	return &Dev{f, 1, blockSize}
}

func (dev *Dev) NewBlock(b *Buf) {
	b.Blkno = dev.next; dev.next++
	for i := 0; i < len(b.Data); i++ {
		b.Data[i] = 0
	}
}

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

func NewCache(filename string, num_bufs, blockSize int64) *Cache {
	dev := OpenDev(filename, blockSize)
	if dev == nil { return nil }
	bufs := make([]Buf, num_bufs)
	data := make([]byte, num_bufs * blockSize)
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
