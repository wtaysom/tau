/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>

#include <debug.h>
#include <eprintf.h>
#include <puny.h>
#include <style.h>
#include <timer.h>
#include <twister.h>

/* test pseudo-random number generators */

typedef u64 (*rnd_f)(void);

static void timer (char *name, rnd_f rnd)
{
	u64 start;
	u64 finish;
	u64 sum = 0;
	unint i = Option.iterations;

	start = nsecs();
	while (i--) {
		sum += rnd();
	}
	finish = nsecs();
	printf("%-10s: %g nsecs/rnd sum=%lld\n",
		name,
		(double)(finish - start) / (double)Option.iterations,
		sum);
}

#define TIMER(_rng)	{						\
	u64 start;							\
	u64 finish;							\
	u64 sum = 0;							\
	unint i = Option.iterations;					\
									\
	start = nsecs();						\
	while (i--) {							\
		sum += _rng;						\
	}								\
	finish = nsecs();						\
	printf("%-20s: %g nsecs/rnd sum=%lld\n",			\
		# _rng,							\
		(double)(finish - start) / (double)Option.iterations,	\
		sum);							\
}


int main (int argc, char *argv[])
{
	unsigned long long init[4]={0x12345ULL, 0x23456ULL,
					0x34567ULL, 0x45678ULL}, length=4;
	init_by_array64(init, length);

	punyopt(argc, argv, NULL, NULL);
	timer("random", (rnd_f)random);
	timer("rand", (rnd_f)rand);
	timer("twister", (rnd_f)prandom);
	TIMER(prandom());
	return 0;
}
