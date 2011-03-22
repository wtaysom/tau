// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"fmt"
	"runtime"
)

func test(e bool) {
	if (e) { return }
	_, file, line, _ := runtime.Caller(1);
	fmt.Printf("Error occured at %s<%d>\n", file, line)
}

func main() {
	_, file, line, _ := runtime.Caller(0);
	fmt.Printf("Error occured at %s<%d>\n", file, line)
	test(3==4)
}
