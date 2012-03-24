package main

import (
	"fmt"
	"websocket"
	"io"
	"http"
)

// Echo the data received on the Web Socket.
func EchoServer(ws *websocket.Conn) {
	fmt.Print("Here\n")
	io.Copy(ws, ws);
}

func main() {
	http.Handle("/echo", websocket.Handler(EchoServer));
	err := http.ListenAndServe(":12345", nil);
	if err != nil {
		panic("ListenAndServe: " + err.String())
	}
}
