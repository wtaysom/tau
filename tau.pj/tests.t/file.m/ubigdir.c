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
#include <puny.h>
#include <eprintf.h>

void usage (void)
{
	pr_usage("-d<directory> -k<files_per_iteration> -i<num_iterations>");
}

int Numfiles = 1000;

void myopt (int c)
{
	switch (c) {
	case 'k':
		Numfiles = strtoll(optarg, NULL, 0);
		break;
	default:
		usage();
		break;
	}
}

int main (int argc, char *argv[])
{
	char		*directory;
	char		name[256];
	unsigned	i, j;
	unsigned	n;
	int		fd;

	punyopt(argc, argv, myopt, "k:");
	directory = Option.dir;
	n = Numfiles;
	mkdirq(directory);
	chdirq(directory);
	for (j = 0; j < Option.iterations; ++j) {
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
