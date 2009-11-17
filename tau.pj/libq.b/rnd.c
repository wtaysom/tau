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

long range (long max)
{
	return max ? (random() % max) : 0;
}

int percent (int x)
{
	return random() % 100 < x;
}

long exp_dist (long range)
{
	if (!range) return 0;
	return range / (random()%range + 1);
}
