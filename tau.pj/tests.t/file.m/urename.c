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
 * RENAME test
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
#include <puny.h>

enum { MAX_NAME = 255 };

char	NameA[MAX_NAME];
char	NameB[MAX_NAME];
char	NameC[MAX_NAME];

void make_names (char *name)
{
	int	fd;

	cat(NameA, name, "_A", NULL);
	cat(NameB, name, "_B", NULL);
	cat(NameC, name, "_C", NULL);
	fd = open(NameA, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	close(fd);
}

void usage (void)
{
	pr_usage("-f<base_file_name> -i<num_iterations> -l<loops>");
}

int main (int argc, char *argv[])
{
	int		rc;
	unsigned	i;
	unsigned	n = 1000;
	u64		l;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	make_names(Option.file);
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			rc = rename(NameA, NameB);
			if (rc == -1) {
				perror("renameAB");
				exit(1);
			}
			rc = rename(NameB, NameC);
			if (rc == -1) {
				perror("renameBC");
				exit(1);
			}
			rc = rename(NameC, NameA);
			if (rc == -1) {
				perror("renameCA");
				exit(1);
			}
		}
		stopTimer();
		prTimer();
		printf("\n");
	}
	return 0;
}
