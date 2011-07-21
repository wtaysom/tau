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
 * getpid microbenchmark
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <puny.h>

void usage (void)
{
	pr_usage("-i<num_iterations> -l<loops>");
}

int main (int argc, char *argv[])
{
	unsigned	i;
	unsigned	n;
	pid_t		pid;
	u64		l;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			pid = getpid();
			if (!pid) {
				fprintf(stderr, "getpid is zero\n");
				exit(1);
			}
		}
		stopTimer();
		printf("n=%d ", n);
		prTimer();
		printf("\n");
	}
	return 0;
}
