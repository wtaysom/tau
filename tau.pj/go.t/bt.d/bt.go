// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

// Do record based opeations
//	Get the record slice, then from from record slice, get key
//	and value
//	Can go to 3 byte lengths or even utf8 lengths
//	3 byte lengths would let me use multi-megabyte blocks
//	and megabyte length records
//
// *1. Split
// *2. Determine split point
// 3. Compaction - is it really full?
// 4. Allocation
// 5. Where do we put the split leaf
// 6. Iterator
// *7. lower case leaf -> leaf
// 8. Use map to store records that are in test
// 9. Need to write out pages - use defer
// 10. Lookup
// 11. Next (this will be my iterator)
// 12. Use strings instead of byte slices
// 13. Delete BufSize from buf and use one passed to NewCache

package main

import (
	"buf"
	"bug"
	"bytes"
	"fmt"
	"rand"
	"time"
	"unsafe"
)

const (
	_ = iota
	LEAF
	BRANCH
)

const (
	LenBuf  = 256
	NumBufs = 10
)

type Len uint16

const (
	LenLen = Len(unsafe.Sizeof(Len(0)))
	LenBlockNum = Len(unsafe.Sizeof(int64(0)))
)

type head struct {
	kind      byte
	numrecs   Len
	available Len
	end       Len
	block     int64
	overflow  int64
	last      int64
}

type Leaf struct {
	b     *buf.Buf
	head  *head
	btree *Btree
}

type Branch struct {
	b     *buf.Buf
	head  *head
	btree *Btree
}

type Btree struct {
	cache *buf.Cache
	root  int64
}

var (
	LenHead Len
	LenRec  Len
)

func init() {
	LenHead = Len(unsafe.Sizeof(head{0, 0, 0, 0, 0, 0, 0}))
	LenRec = Len(3 * LenLen)
}

type Indenter int

func (indent *Indenter) Indent() {
	for i := 0; i < int(*indent); i++ {
		fmt.Print("  ")
	}
}

func (head *head) print(indent Indenter) {
	indent.Indent()
	fmt.Println("numrecs", head.numrecs,
		"available", head.available,
		"end", head.end,
		"block", head.block,
		"overflow", head.overflow,
		"last", head.last)
}

///////////////////////////////////////////////////////////////
//	+---+------+---+-------+
//	|len| key  |len| value |
//	|key|      |val|       |
//	+---+------+---+-------+
func (leaf *Leaf) newLeaf() *Leaf {
	return leaf.btree.newLeaf()
}

func (leaf *Leaf) getLeaf(block int64) *Leaf {
	return leaf.btree.getLeaf(block)
}

func (leaf *Leaf) getLen(offset Len) (length Len, newoffset Len) {
	return Len(leaf.b.Data[offset]) | (Len(leaf.b.Data[offset+1]) << 8),
		offset + LenLen
}

func (leaf *Leaf) ithRec(i Len) Len {
	if i > leaf.head.numrecs {
		bug.Fatal("numrecs =", leaf.head.numrecs, "i =", i)
	}
	return LenHead + LenLen*i
}

func (leaf *Leaf) freeSpace() Len {
	sum := LenHead
	for i := Len(0); i < leaf.head.numrecs; i++ {
		sum += leaf.getRecLen(i) + LenLen
	}
	return LenBuf - sum
}

func (leaf *Leaf) storeLen(offset Len, x Len) Len {
	leaf.b.Data[offset] = byte(x)
	leaf.b.Data[offset+1] = byte(x >> 8)
	return offset + LenLen
}

func (leaf *Leaf) storeSlice(s []byte) {
	n := Len(len(s))
	if leaf.head.available < n+LenLen {
		bug.Fatal("No available space")
	}
	leaf.head.available -= n + LenLen
	leaf.head.end -= n + LenLen
	offset := leaf.storeLen(leaf.head.end, n)
	copy(leaf.b.Data[offset:], s)
}

func (leaf *Leaf) storeIndex(i, offset Len) {
	if i > leaf.head.numrecs {
		bug.Fatal("numrecs =", leaf.head.numrecs, "i =", i)
		return
	}
	start := leaf.ithRec(i)
	if i < leaf.head.numrecs {
		end := leaf.ithRec(leaf.head.numrecs)
		copy(leaf.b.Data[start+LenLen:end+LenLen], leaf.b.Data[start:end])
	}
	leaf.storeLen(start, offset)
	leaf.head.numrecs++
	leaf.head.available -= LenLen
}

func (leaf *Leaf) storeRec(i Len, rec []byte) {
	n := Len(len(rec))
	if n >= leaf.head.available {
		bug.Fatal("leaf full")
	}
	leaf.head.available -= n
	leaf.head.end -= n
	copy(leaf.b.Data[leaf.head.end:], rec)
	leaf.storeIndex(i, leaf.head.end)
}

func (leaf *Leaf) getSlice(offset Len) []byte {
	length, offset := leaf.getLen(offset)
	return leaf.b.Data[offset : offset+length]
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
	vallen, valoffset := leaf.getLen(offset + keylen)
	return leaf.b.Data[offset : valoffset+vallen]
}

func (leaf *Leaf) getRecLen(i Len) Len {
	offset, _ := leaf.getLen(leaf.ithRec(i))
	keylen := leaf.getKeyLen(offset)
	vallen, _ := leaf.getLen(offset + keylen)
	return keylen + vallen + LenLen
}

func (leaf *Leaf) isLE(key []byte, i Len) bool {
	target := leaf.getKey(i)
	return bytes.Compare(key, target) <= 0
}

func (leaf *Leaf) findIndex(key []byte) Len {
	var x, left, right int // left and right must be signed
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

func (leaf *Leaf) split() (right *Leaf) {
	middle := leaf.head.numrecs / 2
	right = leaf.newLeaf()
	j := Len(0)
	for i := Len(middle); i < leaf.head.numrecs; i++ {
		right.storeRec(j, leaf.getRec(i))
		j++
	}
	// Need to count how much space we are using and calculate free space
	leaf.head.numrecs = middle
	leaf.head.overflow = right.head.block
	return right
}

func (leaf *Leaf) copy() *Leaf {
	newleaf := leaf.newLeaf()
	for i := Len(0); i < leaf.head.numrecs; i++ {
		newleaf.storeRec(i, leaf.getRec(i))
	}
	return newleaf
}

func (leaf *Leaf) compact() {
	b := buf.Scratch(LenBuf)
	t := Leaf{b, (*head)(unsafe.Pointer(&b.Data[0])), nil}
	t.head.kind = LEAF
	t.head.available = LenBuf - LenHead
	t.head.end = LenBuf
	t.head.block = leaf.head.block
	t.head.overflow = leaf.head.overflow
	for i := Len(0); i < leaf.head.numrecs; i++ {
		t.storeRec(i, leaf.getRec(i))
	}
	copy(leaf.b.Data, t.b.Data)
}

func (leaf *Leaf) find(key []byte) []byte {
	if leaf.head.overflow != 0 {
		if !leaf.isLE(key, leaf.head.numrecs-1) {
			leaf = leaf.getLeaf(leaf.head.overflow)
		}
	}
	i := leaf.findIndex(key)
	val := leaf.getVal(i)
	rtnval := make([]byte, len(val))
	copy(rtnval, val)
	return rtnval
}

func (leaf *Leaf) insert(key, val []byte) {
	n := Len(len(key)+len(val)) + LenRec
	if n > leaf.head.available {
		right := leaf.split()
		leaf.compact()
		if !leaf.isLE(key, leaf.head.numrecs - 1) {
			leaf = right
		}
	}
	i := leaf.findIndex(key)
	leaf.storeSlice(val)
	leaf.storeSlice(key)
	leaf.storeIndex(i, leaf.head.end)
}

func (leaf *Leaf) isSplit() bool {
	return leaf.head.overflow != 0
}

func (leaf *Leaf) grow() *Branch {
	if !leaf.isSplit() { panic("trying to grow an unsplit leaf") }
	if leaf.head.block != leaf.btree.root { panic("leaf block not root") }
	branch := leaf.btree.newBranch()
	key := leaf.getKey(leaf.head.numrecs - 1)
	branch.store(0, key, leaf.head.block)
	branch.head.last = leaf.head.overflow
	leaf.head.overflow = 0
	leaf.btree.root = branch.head.block
branch.print(3)
bug.Pr(branch.b.Data)
	return branch
}

func (leaf *Leaf) printRec(indent Indenter, i Len) {
	indent.Indent()
	fmt.Printf("%3d. %s %s\n", i, leaf.getKey(i), leaf.getVal(i))
}

func (leaf *Leaf) print(indent Indenter) {
	for {
		leaf.head.print(indent)
		fmt.Println("                free", leaf.freeSpace())
		for i := Len(0); i < leaf.head.numrecs; i++ {
			leaf.printRec(indent, i)
		}
		if leaf.head.overflow == 0 {
			break;
		}
		leaf = leaf.getLeaf(leaf.head.overflow)
		fmt.Println("Overflow")
	}
}
///////////////////////////////////////////////////////////////

//	+---+------+-------+
//	|len| key  | block |
//	|key|      |       |
//	+---+------+-------+
func (branch *Branch) newBranch() *Branch {
	return branch.btree.newBranch()
}

func (branch *Branch) getBranch(block int64) *Branch {
	return branch.btree.getBranch(block)
}

func (branch *Branch) getLen(offset Len) (length Len, newoffset Len) {
	return Len(branch.b.Data[offset]) | (Len(branch.b.Data[offset+1]) << 8),
		offset + LenLen
}

func (branch *Branch) ithRec(i Len) Len {
	if i > branch.head.numrecs {
		bug.Fatal("numrecs =", branch.head.numrecs, "i =", i)
	}
	return LenHead + LenLen*i
}

func (branch *Branch) freeSpace() Len {
	sum := LenHead
	for i := Len(0); i < branch.head.numrecs; i++ {
		sum += branch.getRecLen(i) + LenLen
	}
	return LenBuf - sum
}

func (branch *Branch) storeLen(offset Len, x Len) Len {
	branch.b.Data[offset] = byte(x)
	branch.b.Data[offset+1] = byte(x >> 8)
	return offset + LenLen
}

func (branch *Branch) storeSlice(s []byte) {
	n := Len(len(s))
	if branch.head.available < n+LenLen {
		bug.Fatal("No available space")
	}
	branch.head.available -= n + LenLen
	branch.head.end -= n + LenLen
	offset := branch.storeLen(branch.head.end, n)
	copy(branch.b.Data[offset:], s)
}

func (branch *Branch) storeIndex(i, offset Len) {
	if i > branch.head.numrecs {
		bug.Fatal("numrecs =", branch.head.numrecs, "i =", i)
		return
	}
	start := branch.ithRec(i)
	if i < branch.head.numrecs {
		end := branch.ithRec(branch.head.numrecs)
		copy(branch.b.Data[start+LenLen:end+LenLen], branch.b.Data[start:end])
	}
	branch.storeLen(start, offset)
	branch.head.numrecs++
	branch.head.available -= LenLen
}

func (branch *Branch) storeRec(i Len, rec []byte) {
	n := Len(len(rec))
	if n >= branch.head.available {
		bug.Fatal("branch full")
	}
	branch.head.available -= n
	branch.head.end -= n
	copy(branch.b.Data[branch.head.end:], rec)
	branch.storeIndex(i, branch.head.end)
}

func (branch *Branch) store(i Len, key []byte, block int64) {
bug.Pr("key", string(key), "block", block)
	for j := Len(0); j < LenBlockNum; j++ {
		branch.head.end--
		branch.b.Data[branch.head.end] = byte(block)
		block >>= 8
	}
	branch.storeSlice(key)
	branch.storeIndex(i, branch.head.end)
}

func (branch *Branch) getSlice(offset Len) []byte {
	length, offset := branch.getLen(offset)
	return branch.b.Data[offset : offset+length]
}

func (branch *Branch) getKeyLen(offset Len) Len {
	length, _ := branch.getLen(offset)
	return length + LenLen
}

func (branch *Branch) getKey(i Len) []byte {
	offset, _ := branch.getLen(branch.ithRec(i))
	return branch.getSlice(offset)
}

func (branch *Branch) getVal(i Len) (block int64) {
	offset, _ := branch.getLen(branch.ithRec(i))
	offset += branch.getKeyLen(offset)
	block = 0
	for j := Len(0); j < LenBlockNum; j++ {
		block <<= 8
		block += int64(branch.b.Data[offset]) & 0xff;
		offset++
	}
	return block
}

func (branch *Branch) getRec(i Len) []byte {
	offset, _ := branch.getLen(branch.ithRec(i))
	keylen := branch.getKeyLen(offset)
	return branch.b.Data[offset : offset + keylen + LenBlockNum]
}

func (branch *Branch) getRecLen(i Len) Len {
	offset, _ := branch.getLen(branch.ithRec(i))
	keylen := branch.getKeyLen(offset)
	return keylen + LenBlockNum + LenLen
}

func (branch *Branch) isLE(key []byte, i Len) bool {
	target := branch.getKey(i)
	return bytes.Compare(key, target) <= 0
}

func (branch *Branch) findIndex(key []byte) Len {
	var x, left, right int // left and right must be signed
	left = 0
	right = int(branch.head.numrecs) - 1
	for left <= right {
		x = (left + right) / 2
		if branch.isLE(key, Len(x)) {
			right = x - 1
		} else {
			left = x + 1
		}
	}
	return Len(left)
}

func (branch *Branch) split() (right *Branch) {
	middle := branch.head.numrecs / 2
	right = branch.newBranch()
	j := Len(0)
	for i := Len(middle); i < branch.head.numrecs; i++ {
		right.storeRec(j, branch.getRec(i))
		j++
	}
	// Need to count how much space we are using and calculate free space
	branch.head.numrecs = middle
	branch.head.overflow = right.head.block
	return right
}

func (branch *Branch) copy() *Branch {
	newbranch := branch.newBranch()
	for i := Len(0); i < branch.head.numrecs; i++ {
		newbranch.storeRec(i, branch.getRec(i))
	}
	return newbranch
}

func (branch *Branch) compact() {
	b := buf.Scratch(LenBuf)
	t := Branch{b, (*head)(unsafe.Pointer(&b.Data[0])), nil}
	t.head.kind = LEAF
	t.head.available = LenBuf - LenHead
	t.head.end = LenBuf
	t.head.block = branch.head.block
	t.head.overflow = branch.head.overflow
	for i := Len(0); i < branch.head.numrecs; i++ {
		t.storeRec(i, branch.getRec(i))
	}
	copy(branch.b.Data, t.b.Data)
}

func (branch *Branch) find(key []byte) int64 {
	if branch.head.overflow != 0 {
		if !branch.isLE(key, branch.head.numrecs-1) {
			branch = branch.getBranch(branch.head.overflow)
		}
	}
	i := branch.findIndex(key)
	return branch.getVal(i)
}

func (branch *Branch) insert(key, val []byte) {
	i := branch.findIndex(key)
	if i == branch.head.numrecs {
		child := branch.head.last
	} else {
		child := branch.getVal(i)
	}

	n := Len(len(key)+len(val)) + LenRec
	if n > branch.head.available {
		right := branch.split()
		branch.compact()
		if !branch.isLE(key, branch.head.numrecs - 1) {
			branch = right
		}
	}
	i := branch.findIndex(key)
	branch.storeSlice(val)
	branch.storeSlice(key)
	branch.storeIndex(i, branch.head.end)
}

func (branch *Branch) isSplit() bool {
	return branch.head.overflow != 0
}

func (branch *Branch) grow() *Branch {
	if !branch.isSplit() { panic("trying to grow an unsplit branch") }
	if branch.head.block != branch.btree.root { panic("branch block not root") }
	newbranch := branch.btree.newBranch()
	key := branch.getKey(branch.head.numrecs - 1)
	newbranch.store(0, key, branch.head.block)
	newbranch.head.last = branch.head.overflow
	branch.head.overflow = 0
	branch.btree.root = newbranch.head.block
	return newbranch
}

func (branch *Branch) printRec(indent Indenter, i Len) {
	indent.Indent()
	fmt.Printf("%3d. %s %d\n", i, branch.getKey(i), branch.getVal(i))
}

func (branch *Branch) print(indent Indenter) {
	for {
		branch.head.print(indent)
		fmt.Println("                free", branch.freeSpace())
		for i := Len(0); i < branch.head.numrecs; i++ {
			branch.printRec(indent, i)
		}
		if branch.head.overflow == 0 {
			break;
		}
		branch = branch.getBranch(branch.head.overflow)
		fmt.Println("Overflow")
	}
}

///////////////////////////////////////////////////////////////

func NewBtree(devname string) *Btree {
	btree := new(Btree)
	btree.cache = buf.NewCache(devname, NumBufs, LenBuf)
	return btree
}

func (btree *Btree) newLeaf() *Leaf {
	leaf := new(Leaf)
	leaf.b = btree.cache.NewBuf()
	leaf.btree = btree
	leaf.head = (*head)(unsafe.Pointer(&leaf.b.Data[0]))
	leaf.head.kind = LEAF
	leaf.head.available = LenBuf - LenHead
	leaf.head.end = LenBuf
	leaf.head.block = leaf.b.Blkno
	leaf.head.overflow = 0
	return leaf
}

func (btree *Btree) newBranch() *Branch {
	branch := new(Branch)
	branch.b = btree.cache.NewBuf()
	branch.btree = btree
	branch.head = (*head)(unsafe.Pointer(&branch.b.Data[0]))
	branch.head.kind = BRANCH;
	branch.head.available = LenBuf - LenHead
	branch.head.end = LenBuf
	branch.head.block = branch.b.Blkno
	branch.head.overflow = 0
	branch.head.last = 0
	return branch
}

func (btree *Btree) getLeaf(block int64) *Leaf {
	leaf := new(Leaf)
	leaf.b = btree.cache.GetBuf(block)
	leaf.btree = btree
	leaf.head = (*head)(unsafe.Pointer(&leaf.b.Data[0]))
	return leaf
}

func (btree *Btree) getBranch(block int64) *Branch {
	branch := new(Branch)
	branch.b = btree.cache.GetBuf(block)
	branch.btree = btree
	branch.head = (*head)(unsafe.Pointer(&branch.b.Data[0]))
	return branch
}

func (btree *Btree) Find(key []byte) []byte {
	if btree.root == 0 {
		return nil
	}
	b := btree.cache.GetBuf(btree.root)
	if b == nil {
		bug.Fatal("Couldn't find root")
	}
	head := (*head)(unsafe.Pointer(&b.Data[0]))
	switch head.kind {
	case BRANCH:
	case LEAF:
		return (&Leaf{b, head, btree}).find(key)
	default:
		bug.Fatal("Btree bad root node")
	}
	return nil
}

func (btree *Btree) Insert(key, val []byte) {
	if btree.root == 0 {
		leaf := btree.newLeaf()
		btree.root = leaf.head.block
		leaf.b.Put()
	}
	b := btree.cache.GetBuf(btree.root)
	if b == nil {
		bug.Fatal("Couldn't find root")
	}

	head := (*head)(unsafe.Pointer(&b.Data[0]))
	switch head.kind {
	case BRANCH:
		branch := Branch{b, head, btree}
		branch.insert(key, val)
		if branch.isSplit() {
			branch := branch.grow()
			branch.insert(key, val)
		} else {
			branch.insert(key, val)
		}
	case LEAF:
		leaf := Leaf{b, head, btree}
		if leaf.isSplit() {
			branch := leaf.grow()
			branch.insert(key, val)
		} else {
			leaf.insert(key, val)
		}
	default:
		bug.Fatal("Btree bad root node")
	}
}

func (btree *Btree) Print() {
fmt.Println("Print B-tree");
	if btree.root == 0 {
		fmt.Println("Empty B-tree")
		return
	}
	b := btree.cache.GetBuf(btree.root)
	if b == nil {
		bug.Fatal("Couldn't find root")
	}

	head := (*head)(unsafe.Pointer(&b.Data[0]))
	switch head.kind {
	case BRANCH:
		(&Branch{b, head, btree}).print(0)
	case LEAF:
		(&Leaf{b, head, btree}).print(0)
	default:
		bug.Fatal("Btree bad root node")
	}
}

///////////////////////////////////////////////////////////////
func rndSlice() []byte {
	n := rand.Intn(7) + 5
	s := make([]byte, n)
	for j := 0; j < n; j++ {
		s[j] = byte('a' + rand.Intn(26))
	}
	return s
}

func rndString() string {
	n := rand.Intn(7) + 5
	s := make([]byte, n)
	for j := 0; j < n; j++ {
		s[j] = byte('a' + rand.Intn(26))
	}
	return string(s)
}

type Records map[string]string

func (records Records) dump(btree *Btree) {
	for key, val := range records {
		stored := btree.Find([]byte(key))
		if bytes.Equal(stored, []byte(val)) {
			fmt.Println(key, "=", string(stored))
		} else {
			bug.Error(key, "=", string(stored), "!=", val)
		}
	}
}

func test4() {
	records := make(Records)//make(map[string]string)
	if false { rand.Seed(time.Nanoseconds()) }
	btree := NewBtree(".btfile")
	for i := Len(0); i < 12; i++ {
		key := rndSlice()
		val := rndSlice()
		records[string(key)] = string(val)
		btree.Insert(key, val)
	}
	fmt.Println(records)
	btree.Print()
	records.dump(btree)
}

func main() {
	//	if x := 3; x > 0 { return }
	test4()
}
