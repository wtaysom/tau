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
 * CREATE test
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mystdlib.h>
#include <puny.h>
#include <eprintf.h>

void usage (void)
{
	pr_usage("-f<file_name> -i<num_iterations>");
}

int main (int argc, char *argv[])
{
	char		*name;
	int		fd;
	int		rc;
	unsigned	i;
	unsigned	n;
	u64		l;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	name = Option.file;
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (fd == -1) {
				perror("open");
				exit(1);
			}
			close(fd);
			rc = unlink(name);
			if (rc == -1) {
				perror("unlink");
				exit(1);
			}
		}
		stopTimer();
		prTimer();
		printf("\n");
	}
	return 0;
}
