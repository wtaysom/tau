/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <stdlib.h>

#include <sys/time.h>

void seed_random (void)
{
	struct timeval	time;

	gettimeofday( &time, NULL);
	srandom(time.tv_usec);
}

/* Uniform random integer from 0 to upper-1 */
unsigned long urand (unsigned long upper)
{
	return upper ? (random() % upper) : 0;
}

/*
 * Same as above with own seed but a more limited range
 * because it uses rand_r instead of random.
 */
unsigned urand_r (unsigned upper, unsigned *seedp)
{
	return upper ? (rand_r(seedp) % upper) : 0;
}

int percent (int x)
{
	return random() % 100 < x;
}

long exp_dist (long upper)
{
	if (!upper) return 0;
	return upper / (random() % upper + 1);
}
