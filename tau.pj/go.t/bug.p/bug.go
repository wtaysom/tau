// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package bug

import (
	"fmt"
	"path"
	"runtime"
)

func Here() {
	_, file, line, _ := runtime.Caller(1);
	base := path.Base(file)
	fmt.Printf("%s<%d>\n", base, line)
}

func Pr(a ...interface{}) {
	_, file, line, _ := runtime.Caller(1);
	base := path.Base(file)
	fmt.Printf("%s<%d>", base, line)
	fmt.Println(a...)
}

func Error(a ...interface{}) {
	_, file, line, _ := runtime.Caller(1);
	base := path.Base(file)
	fmt.Printf("Error: %s<%d>", base, line)
	fmt.Println(a...)
}

func Fatal(a ...interface{}) {
	_, file, line, _ := runtime.Caller(1);
	base := path.Base(file)
	fmt.Printf("Fatal: %s<%d>", base, line)
	fmt.Println(a...)
	panic("Fatal Error")
}
