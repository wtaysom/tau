package main

import (
//	"fmt"
	"http"
//	"io"
	"websocket"
)

// Echo the data received on the Web Socket.
func EchoServer(ws *websocket.Conn) {
//	fmt.Printf("Here\n")
	msg := []byte("hello, world\n")
	ws.Write(msg)
//	io.Copy(ws, ws);
}

func main() {
	http.Handle("/echo", websocket.Handler(EchoServer));
	err := http.ListenAndServe(":8080", nil);
	if err != nil {
		panic("ListenAndServe: " + err.String())
	}
}
