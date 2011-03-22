// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package btree

import (
	"buf"
	"bug"
	"bytes"
	"fmt"
	"leaf"
	"unsafe"
)

const (
	_ = iota
	LEAF
	BRANCH
)

type Btree struct {
	Cache		*buf.Cache
	rootblock	int64
}

func (btree *Btree) Print() {
	if btree.rootblock == 0 { return }
	b := btree.Cache.GetBuf(btree.rootblock)
	if b == nil {
		fmt.Println("btree didn't find root")
		return
	}
	head := (*Head)(unsafe.Pointer(&b.Data[0]))
	switch (head.kind) {
	case LEAF:
		var leaf Leaf
		leaf.b = b
		leaf.head = head
		leaf.rec = leaf.recSlice();
		leaf.Print()
	case BRANCH:
		var branch Branch
		branch.b = b
		branch.head = head
		branch.twig = *(*[]Twig)(unsafe.Pointer(&b.Data[sizeHead]))
		branch.Print()
	default:
		bug.Pr("Not a leaf or branch", head.kind)
	}
}

func (btree *Btree) Insert(key, value []byte) {
	if len(key) + len(value) > MAX_KEY_VAL {
		bug.Pr("kay/value too big %d", len(key) + len(value))
		return;
	}
	if btree.rootblock == 0 {
		leaf := btree.newLeaf()
		btree.rootblock = leaf.head.block
		leaf.b.Put()
	}
	b := btree.Cache.GetBuf(btree.rootblock)
	if b == nil {
		fmt.Println("btree didn't find root")
		return
	}
	head := (*Head)(unsafe.Pointer(&b.Data[0]))
	if head.overflow != 0 {
		bug.Pr("Need to grow tree");
		return;
	}
	switch (head.kind) {
	case LEAF:
		var leaf Leaf
		leaf.b = b
		leaf.head = head
		leaf.rec = leaf.recSlice();
		leaf.insert(key, value)
	case BRANCH:
		var branch Branch
		branch.b = b
		branch.head = head
		branch.twig = *(*[]Twig)(unsafe.Pointer(&b.Data[sizeHead]))
		branch.insert(key, value)
	default:
		bug.Pr("Not a leaf or branch", b.Data[0])
	}
}
