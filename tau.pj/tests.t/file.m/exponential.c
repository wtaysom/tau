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

long exp_dist (long range)
{
	return range / (random()%range + 1);
}

#include <stdio.h>

int main (int argc, char *argv[])
{
	int	i;
	long	x;

	for (;;) {
		for (i = 0; i < 1000; ++i) {
			x = exp_dist(1<<10);
			q_sum(x);
		}
		pr_sum("exp dist");
		printf("\n");
	}
	return 0;
}
