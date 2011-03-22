// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"fmt"
	"math"
)

func sqrt(x float64) float64 {
	var w float64
	z := x / 2
	for i := 0; i < 10 ; i++ {
		w = z - (z * z - x) / (2*z)
fmt.Println(i, x, w, z)
		if w == z { break; }
		z = w
	}
	return z
}

func main() {
	var x float64
	for x = 1.0; x < 1000.0; x++ {
		fmt.Printf("  %g %g %g\n", x, sqrt(x), math.Sqrt(x))
	}
}
