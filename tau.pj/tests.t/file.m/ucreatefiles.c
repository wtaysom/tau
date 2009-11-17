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
 * WRITE FILES test -- write many files.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <style.h>
#include <mylib.h>
#include <myio.h>

void usage (char *name)
{
	fprintf(stderr,
		"%s <directory> [<num_iterations> [<prefix> [<start>]]]\n",
		name);
	exit(1);
}

int main (int argc, char *argv[])
{
	char		*directory = "";
	char		*prefix = "";
	char		name[256];
	int		fd;
	unsigned	i, j;
	unsigned	start = 0;
	unsigned	n = 1000;

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
		prefix = argv[3];
	}
	if (argc > 4) {
		start = atoi(argv[4]);
	}
	seed_random();

	mkdir(directory, 0777);
	chdirq(directory);
	for (j = 0; ; ++j) {
		startTimer();
		for (i = 0; i < n; ++i) {
			if (start) {
				sprintf(name, "%s%x", prefix, ++start);
			} else {
				sprintf(name, "%s%lx", prefix, random());
			}
			fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (fd == -1) {
				perror(name);
				exit(1);
			}
			close(fd);
		}
		stopTimer();
		prTimer();
		printf(" n=%d\n", n);
	}
}
