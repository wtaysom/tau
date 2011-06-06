
package main

import (
	"fmt"
)

func main() {
	for k := 2.5; k < 4.0; k += 0.1 {
		x := .5
		fmt.Printf("k=%g x=%g ************\n", k, x)
		for i := 0; i < 100; i++ {
			x = k * x * (1.0 - x)
			fmt.Printf("%g\n", x)
		}
	}
}
