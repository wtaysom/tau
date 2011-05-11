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
 * FSTAT test
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mylib.h>
#include <puny.h>
#include <eprintf.h>

void usage (void)
{
	pr_usage("-f<file_name> -i<num_iterations> -l<loops>");
}

int main (int argc, char *argv[])
{
	struct stat	sb;
	char		*name;
	int		fd;
	int		rc;
	unsigned	i;
	unsigned	n;
	u64		l;

	punyopt(argc, argv, NULL, NULL);
	name = Option.file;
	n = Option.iterations;
	fd = open(name, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			rc = stat(name, &sb);
			if (rc == -1) {
				perror("stat");
				exit(1);
			}
		}
		stopTimer();
		prTimer();
		printf("\n");
	}
	close(fd);
	return 0;
}
