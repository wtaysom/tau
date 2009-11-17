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
 * Make a big directory with millions of empty files.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mylib.h>
#include <myio.h>

void usage (char *name)
{
	fprintf(stderr, "%s <directory> [<files_per_iteration> [<num_iterations>]]\n",
		name);
	exit(1);
}

int main (int argc, char *argv[])
{
	char		*directory = "";
	char		name[256];
	unsigned	i, j;
	unsigned	n = 1000;
	unsigned	iterations = 1000;
	int		fd;

	if (argc < 2) {
		usage(argv[0]);
	}
	if (argc > 1) {
		directory = argv[1];
	}
	if (argc > 2) {
		n = atoi(argv[2]);
	}
	if (argc > 3) {
		iterations = atoi(argv[3]);
	}
	mkdirq(directory);
	chdirq(directory);
	for (j = 0; j < iterations; ++j) {
		startTimer();
		for (i = 0; i < n; ++i) {
			sprintf(name, "f_%d_%d", j, i);
			fd = openq(name, O_RDWR | O_CREAT | O_TRUNC);
			closeq(fd);
		}
		stopTimer();
		prTimer();
		printf(" j=%d n=%d\n", j, n);
	}
	return 0;
}
