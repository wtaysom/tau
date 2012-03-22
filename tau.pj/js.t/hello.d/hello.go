package main

import (
	"fmt"
	"http"
	"os"
	"math"
	"strconv"
	"time"
)

func sayHello(w http.ResponseWriter, req *http.Request) {
	fmt.Printf("New Request\n")
	fmt.Fprintln(w, "<h1>Go Say's Hello</h1><h2>(Via http)</h2>")
}

func ArgServer(w http.ResponseWriter, req *http.Request) {
	for _, s := range os.Args {
		fmt.Fprintln(w, s)
	}
}

func sineServer(w http.ResponseWriter, req *http.Request) {
	x := 0.
	for {
		if x >= 2*math.Pi {
			x = 0
		} else {
			x += 0.05
		}
		
		time.Sleep(500*1000*1000) // sleep for 500ms (Sleep takes nanoseconds)

		msg := strconv.Ftoa64(math.Sin(x), 'g', 10)
		fmt.Printf("%v sending: %v\n", w, msg)
		w.Write( []byte(msg) )
	}
}


func main(){
	fmt.Printf("Starting http Server ... ")
	http.Handle("/args", http.HandlerFunc(ArgServer))
	http.Handle("/hello", http.HandlerFunc(sayHello))
	http.Handle("/sine", http.HandlerFunc(sineServer))
	err := http.ListenAndServe("0.0.0.0:8080", nil)
	if err != nil {
		fmt.Printf("ListenAndServe Error :" + err.String())
	}
}
