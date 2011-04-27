/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>

#include <debug.h>
#include <eprintf.h>

int func_low(int p1, int p2)
{
	p1 = p1 - p2;
	stacktrace();

	return 2*p1;
}

int func_high(int p1, int p2)
{

	p1 = p1 + p2;
	stacktrace();

	return 2*p1;
}


int test(int p1)
{
	int res;

	if (p1 < 10)
		res = 5+func_low(p1, 2*p1);
	else
		res = 5+func_high(p1, 2*p1);
	return res;
}

int fib(int n)
{
	if (n > 1) {
		return fib(n-1) + fib(n-2);
	} else {
		stacktrace();
		return 1;
	}
}

int fact(int n)
{
	if (n > 1) {
		return n * fact(n-1);
	} else {
		stacktrace();
		return 1;
	}
}

void a(void)
{
	fatal("end of the line");
}

void b(void)
{
	a();
}

void c(void)
{
	b();
}

void d(void)
{
	c();
}

int main(int argc, char *argv[])
{
	printf("First call: %d\n\n", test(27));
	printf("Second call: %d\n\n", test(4));

	printf("Fib %d\n\n", fib(1));
	printf("Fib %d\n\n", fib(9));

	d();

	return 0;
}
