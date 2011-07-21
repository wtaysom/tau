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
 * OPEN test
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <eprintf.h>
#include <mystdlib.h>
#include <puny.h>

void usage (void)
{
	pr_usage("-f<file path> -i<num_iterations> -l<loops>");
}

int main (int argc, char *argv[])
{
	int		fd;
	unsigned	i;
	unsigned	n;
	u64		l;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	fd = creat(Option.file, 0760);
	if (fd == -1) fatal(Option.file);

	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			fd = open(Option.file, O_RDONLY);
			if (fd == -1) {
				perror("open");
				exit(1);
			}
			close(fd);
		}
		stopTimer();
		prTimer();
		printf("\n");
	}
	return 0;
}
