// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"fmt"
	"reflect"
	"unsafe"
)

var I int

type Leaf struct {
	a *int;
	b int;
}

type lenT uint16

type LeafHead struct {
	kind	byte
	numrecs	lenT
	free	lenT
	block	int64
}

type Pnt struct { x, y int }

var L  = LeafHead{0, 0, 0, 0}


var length lenT
var leafhead LeafHead

const (
	sizeLenT = lenT(unsafe.Sizeof(lenT(0)))
)

var (
	sizeHead = unsafe.Sizeof(Pnt{0, 0})
)

func init() {
	sizeHead = unsafe.Sizeof(LeafHead{0, 0, 0, 0})
}

var A = [20]byte{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 , 14, 15, 16, 17 }

type  Rec struct {
	a, c uint16
}

type Head struct {
	n	uint16
	size	uint16
}

func main() {
	var a []byte
	var e []byte
	var r reflect.SliceHeader
	fmt.Println("A", A)
	a = A[2:10]
	fmt.Println("a.cap", cap(a));
	fmt.Println("a", a)
	d := (unsafe.Pointer(&A))
	fmt.Println("d", d)
	r.Data = uintptr(unsafe.Pointer(&A))
	r.Len = 4
	r.Cap = 10
	e = *(*[]byte)(unsafe.Pointer(&r))
	//e = a
	fmt.Println("e", e)
	var rec Rec
	f := unsafe.Sizeof(rec)
	fmt.Println("sizeof rec", f)
/*
	fmt.Println("A", A)
	a := A[2:10]
	fmt.Println("a", a)
	b := unsafe.Pointer(&A)
	fmt.Println("b", b)
	c := unsafe.Pointer(&a)
	fmt.Println("c", c)
	d := (*[2]Rec)(unsafe.Pointer(&a[2]))
	fmt.Println("d", d)
	head := (* Head)(unsafe.Pointer(&A))
	fmt.Println("head", head)
	fmt.Println("*head", *head)
	fmt.Println(unsafe.Sizeof(I))
	fmt.Println(unsafe.Sizeof(leafhead))
	fmt.Println(sizeHead)
*/
}
