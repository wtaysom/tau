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
 * lseek microbenchmark
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mylib.h>
#include <eprintf.h>

void usage (char *name)
{
	fprintf(stderr,
		"%s <file_name> <num_iterations>\n",
		name);
	exit(1);
}

int main (int argc, char *argv[])
{
	char		*name = "";
	int		fd;
	unsigned	i;
	unsigned	n = 1000;
	off_t		rc;

	if (argc < 2) {
		usage(argv[0]);
	}
	if (argc > 1) {
		name = argv[1];
	}
	if (argc > 2) {
		n = atoi(argv[2]);
	}
	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	for (;;) {
		startTimer();
		for (i = 0; i < n; ++i) {
			rc = lseek(fd, i, 0);
			if (rc != i) {
				fprintf(stderr, "lseek offset=%d rc=%lld\n",
					i, (s64)rc);
				exit(1);
			}
		}
		stopTimer();
		printf("n=%d ", n);
		prTimer();
		printf("\n");
	}
	close(fd);
	return 0;
}
