// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"buf"
	"cmd"
	"fmt"
	"leaf"
	"os"
	"rand"
	"runtime"
	"strconv"
)

func test(e bool) {
	if (e) { return }
	_, file, line, _ := runtime.Caller(1)
	fmt.Printf("Error at %s<%d>\n", file, line)
}

func rndName(n int) string {
	b := make([]byte, n)
	for i := 0; i < n; i++ {
		b[i] = byte('a' + rand.Int() % 26)
	}
	return string(b)
}
 
func rndSlice() []byte {
	n := rand.Intn(7) + 5
	s := make([]byte, n)
	for j := 0; j < n; j++ {
		s[j] = byte('a' + rand.Intn(26))
	}
	return s
}


func cachetest(args []string) {
	cachefile := "/tmp/cache"
	if len(args) > 0 {
		cachefile = args[0]
	}
	fmt.Printf("cache file = %s\n", cachefile)
	cache := buf.NewCache(cachefile, 10, 1<<8)
	if cache == nil { return }

// Test 1. Get a buffer, set a value, and Put it back
	b := cache.NewBuf()
	test(b != nil)
	test(b.Blkno == 1)
	b.Data[0] = 'A'
fmt.Printf("b=%p\n", b)
	b.Put()

	cache.Clear()

// Test 2. Get the buffer from test 1 and check it has what we set
// This is not a good test, because it depends on test 1.
// This test did fail to detect that we read the block into a
// separate buffer. Now we have to copies of the buffer in memory.
	b = cache.GetBuf(1)
fmt.Printf("b=%p\n", b)
fmt.Println("b=", b)
	test(b != nil)
	test(b.Blkno == 1)
	test(b.Data[0] == 'A')
	b.Put()
	test(cache.Audit())

// Test 3. Use up all the buffers without releasing any of them
// Can't really test for running off of end nor can we test for
// infinite loops.
	for i := 0; i < len(cache.Buf); i++ {
		b = cache.GetBuf(int64(i))
		b.Put()
	}
	b = cache.GetBuf(int64(len(cache.Buf)));
	test(b != nil);

	test(cache.Audit());
}

func rs(args []string) {
	n := 10
	if len(args) > 0 {
		n, _ = strconv.Atoi(args[0])
	}
	fmt.Printf("%s\n", rndName(n))
}

var Btree btree.Btree

func newtree(args []string) {
	name := ".btree"
	if len(args) > 0 {
		name = args[0];
	}
	Btree.Cache = buf.NewCache(name, 10, 1<<8)
	if Btree.Cache == nil {
		fmt.Fprintln(os.Stderr, "new cache failed", name);
		return
	}
}

func inserttree(args []string) {
	if len(args) < 2 {
		fmt.Fprintln(os.Stderr, "insert requires <name> <value>");
	}
	key := []byte(args[0])
	value := []byte(args[1])

	Btree.Insert(key, value)
}

func btreetest() {
	var mytree btree.Btree
	name := ".btree"
	mytree.Cache = buf.NewCache(name, 10, 1<<8)
	if mytree.Cache == nil {
		fmt.Fprintln(os.Stderr, "new cache failed", name);
		return
	}
	mytree.Insert([]byte("abc"),  []byte("efghij"));
	mytree.Insert([]byte("bcde"), []byte("fghijklmnop"));
	for i := 0; i < 9; i++ {
		mytree.Insert(rndSlice(), rndSlice());
	}
	mytree.Print();
}

func mycmd() {
	//rand.Seed(time.Nanoseconds())
	cmd.Add(cachetest, "cachetest", "test cache code")
	cmd.Add(rs, "rs", "[<len>] print a random string")
	cmd.Add(newtree, "tree", "[<file name>] create a B+tree")
	cmd.Add(inserttree, "insert", "<name> <value> insert into B+tree")
	//cmd.Cmd();
	btreetest();
}

func test4() {
	rand.Seed(time.Nanoseconds())
	lf := leaf.NewLeaf()
	for i := leaf.Len(0); i < 20; i++ {
		key := rndSlice()
		val := rndSlice()
		lf.Insert(key, val)
	}
	leaf.Print(5)
	a := lf.Copy()
	a.Print(10)
}

func main() {
	//	if x := 3; x > 0 { return }
	test4()
}
