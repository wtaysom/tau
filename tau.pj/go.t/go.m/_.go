// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

// Do record based opeations
//	Get the record slice, then from from record slice, get key
//	and value
//	Can go to 3 byte lengths or even utf8 lengths
//	3 byte lengths would let me use multi-megabyte blocks
//	and megabyte length records
//
// 1. Split
// 2. Determine split point
// 3. Compaction - is it really full?
// 4. Allocation
// 5. Where do we put the split leaf
// 6. Iterator

package main

import (
	"bytes"
	"bug"
	"fmt"
	"rand"
	"time"
	"unsafe"
)

type Rec struct {
	a, b, c uint16
}

func test1(n int) {
	var a [3]int
	b := make([]int, n)
	fmt.Println(a, b)
	println("hi")
}

func test2() {
	fmt.Println(unsafe.Sizeof(Rec{0, 0, 0}))
}

func test3() {
	rand.Seed(time.Nanoseconds())
	for i := 0; i < 10; i++ {
		n := rand.Intn(20) + 6
		s := make([]byte, n)
		for j := 0; j < n; j++ {
			s[j] = byte('a') + byte(rand.Intn(26))
		}
		fmt.Println(string(s))
	}
}

const LenBuf = 512

type Len uint16

const LenLen = Len(unsafe.Sizeof(Len(0)))

type Head struct {
	numrecs Len
	free    Len
	end     Len
}

type Leaf struct {
	b    [LenBuf]byte
	head *Head
	rec  []Len
}

var (
	LenHead Len
	LenRec  Len
)

func init() {
	LenHead = Len(unsafe.Sizeof(Head{0, 0, 0}))
	LenRec = Len(3 * LenLen)
}

func NewLeaf() *Leaf {
	leaf := new(Leaf)
	leaf.head = (*Head)(unsafe.Pointer(&leaf.b[0]))
	leaf.head.free = LenBuf - LenHead
	leaf.head.end = LenBuf
	return leaf
}

func (leaf *Leaf) getLen(offset Len) (lenght Len, newoffset Len) {
	return Len(leaf.b[offset]) | (Len(leaf.b[offset+1]) << 8), offset + LenLen
}

func (leaf *Leaf) storeLen(offset Len, x Len) Len {
	leaf.b[offset] = byte(x)
	leaf.b[offset+1] = byte(x >> 8)
	return offset + LenLen
}

func (leaf *Leaf) ithRec(i Len) Len {
	if i > leaf.head.numrecs {
		bug.Fatal("i=%d numrecs=%d", i, leaf.head.numrecs)
	}
	return LenHead + LenLen * i
}

func (leaf *Leaf) storeSlice(s []byte) {
	n := Len(len(s))
	if (leaf.head.free < n + LenLen) {
		bug.Fatal("No free space")
	}
	leaf.head.free -= n + LenLen
	leaf.head.end -= n + LenLen
	offset := leaf.storeLen(leaf.head.end, n)
	copy(leaf.b[offset:], s)
}

func (leaf *Leaf) storeIndex(i, offset Len) {
	if i > leaf.head.numrecs {
		bug.Fatal("numrecs=%d i=%d", leaf.head.numrecs, i)
		return
	}
	start := leaf.ithRec(i)
	if i < leaf.head.numrecs {
		end := leaf.ithRec(leaf.head.numrecs)
		copy(leaf.b[start+LenLen:end+LenLen], leaf.b[start:end])
	}
	leaf.storeLen(start, offset)
	leaf.head.numrecs++
	leaf.head.free -= LenLen
}

func (leaf *Leaf) storeRec(i Len, rec []byte) {
	n := Len(len(rec))
	if (n >= leaf.head.free) {
		bug.Fatal("Leaf full")
	}
	leaf.head.free -= n
	leaf.head.end -= n
	copy(leaf.b[leaf.head.end:], rec)
	leaf.storeIndex(i, leaf.head.end)
}

func (leaf *Leaf) getSlice(offset Len) []byte {
	length, offset := leaf.getLen(offset)
	return leaf.b[offset : offset+length]
}

func (leaf *Leaf) getKeyLen(offset Len) Len {
	length, _ := leaf.getLen(offset)
	return length + LenLen
}

func (leaf *Leaf) getKey(i Len) []byte {
	offset, _ := leaf.getLen(leaf.ithRec(i))
	return leaf.getSlice(offset)
}

func (leaf *Leaf) getVal(i Len) []byte {
	offset, _ := leaf.getLen(leaf.ithRec(i))
	offset += leaf.getKeyLen(offset)
	return leaf.getSlice(offset)
}

func (leaf *Leaf) getRec(i Len) []byte {
	offset, _ := leaf.getLen(leaf.ithRec(i))
	keylen := leaf.getKeyLen(offset)
	vallen, valoffset := leaf.getLen(offset+keylen)
	return leaf.b[offset : valoffset + vallen]
}

func (leaf *Leaf) isLE(key []byte, i Len) bool {
	target := leaf.getKey(i)
	return bytes.Compare(key, target) <= 0
}

func (leaf *Leaf) findIndex(key []byte) Len {
	var x, left, right int	// left and right must be signed
	left = 0
	right = int(leaf.head.numrecs) - 1
	for left <= right {
		x = (left + right) / 2
		if leaf.isLE(key, Len(x)) {
			right = x - 1
		} else {
			left = x + 1
		}
	}
	return Len(left)
}

func (leaf *Leaf) Copy() *Leaf {
	newLeaf := NewLeaf()
	for i := Len(0); i < leaf.head.numrecs; i++ {
		newLeaf.storeRec(i, leaf.getRec(i))
	}
	return newLeaf
}

func (leaf *Leaf) Insert(key, val []byte) {
	n := Len(len(key)+len(val)) + LenRec
	if n > leaf.head.free {
		bug.Error("Out of space")
		return
	}
	i := leaf.findIndex(key)
	leaf.storeSlice(val)
	leaf.storeSlice(key)
	leaf.storeIndex(i, leaf.head.end)
}

type Indenter int

func (indent *Indenter) Indent() {
	for i := 0; i < int(*indent); i++ {
		fmt.Print("  ")
	}
}

func (head *Head) Print(indent Indenter) {
	indent.Indent()
	fmt.Println("numrecs=", head.numrecs, " free=", head.free, " end=", head.end)
}

func (leaf *Leaf) printRec(indent Indenter, i Len) {
	indent.Indent()
	fmt.Printf("%3d. %s %s\n", i, leaf.getKey(i), leaf.getVal(i))
}

func (leaf *Leaf) Print(indent Indenter) {
	leaf.head.Print(indent)
	for i := Len(0); i < leaf.head.numrecs; i++ {
		leaf.printRec(indent, i)
	}
}

func rndSlice() []byte {
	n := rand.Intn(7) + 5
	s := make([]byte, n)
	for j := 0; j < n; j++ {
		s[j] = byte('a' + rand.Intn(26))
	}
	return s
}

func test4() {
	rand.Seed(time.Nanoseconds())
	leaf := NewLeaf()
	for i := Len(0); i < 20; i++ {
		key := rndSlice()
		val := rndSlice()
		leaf.Insert(key, val)
	}
	leaf.Print(5)
	a := leaf.Copy()
	a.Print(10)
}

func main() {
	//	if x := 3; x > 0 { return }
	test4()
}
