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
#include <mylib.h>

#include <puny.h>

long exp_dist (long range)
{
	return range / (random()%range + 1);
}

#include <stdio.h>

int main (int argc, char *argv[])
{
	int	i;
	long	x;
	u64	l;

	punyopt(argc, argv, NULL, NULL);
	for (l = 0; l < Option.loops; l++) {
		for (i = 0; i < Option.iterations; ++i) {
			x = exp_dist(1<<10);
			q_sum(x);
		}
		pr_sum("exp dist");
		printf("\n");
	}
	return 0;
}
