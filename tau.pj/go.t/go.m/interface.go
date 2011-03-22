// Copyright 2010 Google Inc. All Rights reserved
// Author: taysom@google.com (Paul Taysom)

package main

import (
	"fmt"
	//"rand"
	//"time"
	//"unsafe"
)

type stk struct {
	data [10]int
	top int
}

type que struct {
	data [10]int
	head int
	tail int
}

func (s *stk) push(a int) {
	if s.top == len(s.data) {
		fmt.Println("stk full")
		return
	}
	s.data[s.top] = a;
	s.top++
}

func (s *stk) pop() int {
	if s.top == 0 {
		fmt.Println("stk empty");
		return 0
	}
	s.top--
	return s.data[s.top]
}

func (q *que) inc_head() {
	q.head++;
	if q.head == len(q.data) {
		q.head = 0
	}
}

func (q *que) dec_head() {
	if q.head == 0 {
		q.head = len(q.data) - 1
	} else {
		q.head--
	}
}

func (q *que) inc_tail() {
	q.tail++
	if q.tail == len(q.data) {
		q.tail = 0
	}
}

func (q *que) dec_tail() {
	if q.tail == 0 {
		q.tail = len(q.data) - 1
	} else {
		q.tail--
	}
}

func (q *que) push(a int) {
	q.dec_head();
	if q.head == q.tail {
		fmt.Println("queue full")
		q.inc_head();
		return
	}
	q.data[q.head] = a
}

func (q *que) pop()(a int) {
	if q.head == q.tail {
		fmt.Println("queue empty")
		return 0
	}
	a = q.data[q.head]
	q.inc_head()
	return a
}

type can interface {
	push(a int)
	pop() int
}

func t1(c can) {
  c.push(3)
  fmt.Println(c.pop())
}

func main() {
	var s stk
	var q que;
	t1(&s)
	t1(&q)
//	s.push(1)
//	q.push(2)
//	fmt.Println(s.pop(), q.pop())
}
