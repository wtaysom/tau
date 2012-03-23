package main

import (
	"fmt"
	"http"
	"io"
	"log"
	"math"
	"strconv"
	"time"
	"websocket"
)

func main() {
	log.Println("Starting Server...")

	http.Handle("/echo", websocket.Handler(echoServer));
//	http.Handle("/ws", websocket.Handler(handler));
//	http.HandleFunc("/", handlerSimple);

	err := http.ListenAndServe(":8080", nil);
	if err != nil {
		panic("ListenAndServe: " + err.String())
	}
}

func handler(ws *websocket.Conn) {
	fmt.Printf("Here\n")
	x := 0.
	for {
		if x >= 2*math.Pi {
			x = 0
		} else {
			x += 0.05
		}
		
		time.Sleep(500*1000*1000) // sleep for 500ms (Sleep takes nanoseconds)

		msg := strconv.Ftoa64(math.Sin(x), 'g', 10)
		fmt.Printf("%v sending: %v\n", ws, msg)
		ws.Write( []byte(msg) )
	}
}

// Echo the data received on the Web Socket.
func echoServer(ws *websocket.Conn) {
	io.Copy(ws, ws);
}

func handlerSimple(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Hi there, I love %s!", r.URL.Path[1:])
}
