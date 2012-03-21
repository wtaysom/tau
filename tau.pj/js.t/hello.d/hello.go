package main

import (
	"fmt"
	"http"
	"os"
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

func main(){
	fmt.Printf("Starting http Server ... ")
	http.Handle("/args", http.HandlerFunc(ArgServer))
	http.Handle("/hello", http.HandlerFunc(sayHello))
	err := http.ListenAndServe("0.0.0.0:8080", nil)
	if err != nil {
		fmt.Printf("ListenAndServe Error :" + err.String())
	}
}
