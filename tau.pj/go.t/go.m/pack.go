// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"fmt"
	"rand"
	"time"
)
// store a hunk of data

func pack(block, hunk []byte, end int) {
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

func main() {
//	if x := 3; x > 0 { return }
	test3()
}
