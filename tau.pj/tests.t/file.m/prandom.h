/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Problem:
 *	Pseudo random numbers are used as keys for testing
 *	B-tree, binary trees, hashing, etc. The birthday
 *	paradox implies using at least 64 bit values.
 *
 *
 * Code based on David Jones JKISS32 pseudo random generator.
 * See http://www.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf
 *
 * prandom does provide an implementation using a hidden global variables
 * but its been primarily designed to support multiple threads.
 * Requirements:
 *	Fast
 *	Works well on 32 and 64 bit machines
 *	Long period
 *	Repeatable Results
 *	Option to get 64 bit results (to reduce collisions)
 * KISS32
 *	No multiply
 *	Only 32 bit operations
 *	Period approx. 2**121
 *	Only 20 bytes have to be kept per instance of generator
 */

#ifndef _PRANDOM_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <style.h>

typedef struct PrandomSeed_s {
	int w;
	int x;
	int y;
	int z;
	int c;
} PrandomSeed_s;

static PrandomSeed_s Seed = {
	.x = 123456789;
	.y = 234567891;
	.z = 345678912;
	.w = 456789123;
	.c = 0;
};

/* Implementation of a 32-bit KISS generator which uses no multiply instructions */
unsigned int prandom (void)
{
	int t;

	Seed.y ^= (Seed.y << 5);
	Seed.y ^= (Seed.y >> 7);
	Seed.y ^= (Seed.y << 22); 
	t       = Seed.z + Seed.w + Seed.c;
	Seed.z  = Seed.w;
	Seed.c  = t < 0;
	Seed.w  = t & 2147483647;
	Seed.x += 1411392427;
	return Seed.x + Seed.y + Seed.w;
}

unsigned int prandom_r (PrandomSeed_s *seed)
{
	int t;

	seed->y ^= (seed->y << 5);
	seed->y ^= (seed->y >> 7);
	seed->y ^= (seed->y << 22); 
	t        = seed->z + seed->w + seed->c;
	seed->z  = seed->w;
	seed->c  = t < 0;
	seed->w  = t & 2147483647;
	seed->x += 1411392427;
	return seed->x + seed->y + seed->w;
}

PrandomSeed_s 


void init_random_seed (void *seed, int numbytes)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) fatal("open /dev/urandom:");
	ssize_t rc = read(fd, seed, numbytes);
	if (rc != numbytes) fatal("read /dev/urandom:");
}

#endif
