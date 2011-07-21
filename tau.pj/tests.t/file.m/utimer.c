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

/*
 * TIMER test
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <style.h>
#include <mystdlib.h>
#include <puny.h>

int main (int argc, char *argv[])
{
	struct timeval	seed;
	int	x = 0;
	int	rc;
	u32	i;
	u32	n;
	u64	l;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	gettimeofday( &seed, NULL);
	srandom(seed.tv_sec ^ seed.tv_usec);
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			x = urand(1000000);
			rc = usleep(x);
			if (rc == -1) {
				perror("usleep");
				exit(1);
			}
		}
		stopTimer();
		printf("%d micro ", x);
		prTimer();
		printf("\n");
	}
	return 0;
}
