package main
import "fmt"

func a(n int) []byte {
	return make([]byte, n)
}

func main() {
	c := a(4)
	fmt.Println("Size ", len(c))
}
