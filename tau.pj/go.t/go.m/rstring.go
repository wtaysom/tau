package main

import (
	"flag"
	"fmt"
	"rand"
	"strconv"
	"time"
)

func main() {
	flag.Parse()
	n := 10
	if flag.NArg() > 0 {
		n, _ = strconv.Atoi(flag.Arg(0))
	}
	rand.Seed(time.Nanoseconds())
	for i := 0; i < n; i++ {
		fmt.Printf("%c", 'a' + rand.Int() % 26);
	}
	fmt.Printf("\n")
}
