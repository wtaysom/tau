// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"fmt"
)

type Indenter int

func (indent *Indenter) Indent() {
	for i := 0; i < int(*indent); i++ {
		fmt.Print("  ")
	}
}

func test(x Indenter) {
	x.Indent()
	fmt.Println("|")
}

func main() {
	test(0)
	test(3)
	test(5)
}
