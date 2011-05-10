/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Forces deletes/inserts to converge to a specified level then
 * hover around that level.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <style.h>
#include <puny.h>
#include <debug.h>

int Level = 200;
int Count = 0;

int should_dec(void)
{
	enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
	int x = random();
	if (Option.print) printf("%10d ", x);
	x &= MASK;
	if (Option.print) printf("%10d ", x);
	x *= Count;
	if (Option.print) printf("%10d ", x);
	x /= Level;
	if (Option.print) printf("%10d ", x);
	x /= RANGE;
	if (Option.print) printf("%3d ", x);

	return x;
}

void test1(int n)
{
	s64	sum = 0;
	int	i;

	for (i = 0; i < n; i++) {
		if (Option.print) printf("%3d. ", i);
		if (should_dec()) {
			--Count;
		} else {
			++Count;
		}
		sum += Count;
		if (Option.print) printf("%5d ", Count);
		if (Option.print) printf("%g\n", ((double)sum) / (i+1));
	}
	printf("Level = %d avg = %g\n", Level, ((double)sum) / n);
}

void myopt(int c)
{
	switch(c) {
	case 'k':
		Level = strtoll(optarg, NULL, 0);
		break;
	default:
		usage();
		break;
	}
}

int main (int argc, char *argv[])
{
	Option.iterations = 10;
	Level = 2 * Option.iterations;
	punyopt(argc, argv, myopt, "k:");
	test1(Option.iterations);
	return 0;
}
