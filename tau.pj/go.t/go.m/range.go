// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import "fmt"

var A = []string { "apple", "baby", "cat" }

func main() {
	fmt.Println(A);
	for key, value := range(A) {
		fmt.Println(key, value);
	}
}
