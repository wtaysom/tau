// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package cmd

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

type Func func (args []string)
type cmd struct {
	f	Func
	help	string
}

var (
	commands = map[string] cmd {
		"q": cmd{q, "quit" },
		"echo": cmd{echo, "echo <args>" },
	}
)

func q(args []string) {
	os.Exit(0)
}

func help(args []string) {
	for name, cmd := range commands {
		fmt.Printf("%v:\t%v\n", name, cmd.help)
	}
}

func echo(args []string) {
	for _,a := range args {
		fmt.Printf("%s ", a)
	}
	fmt.Printf("\n");
}

func Add(f Func, name string, help string) {
	commands[name] = cmd{f, help}
}

func execute(args []string) {
	if len(args) == 0 { return }
	cmd, ok := commands[args[0]];
	if ok {
		cmd.f(args[1:])
	} else {
		fmt.Println(args[0], "not found");
	}
}

func prompt() {
	fmt.Printf("> ")
}

func Cmd() {
	Add(help, "help", "display help")
	Add(help, "?", "display help")
	in := bufio.NewReader(os.Stdin)
	for {
		prompt()
		s, ok := in.ReadString('\n')
		if ok != nil{
			fmt.Println("exiting", ok);
			return;
		}
		args := strings.Fields(s)
		execute(args)
	}
}
