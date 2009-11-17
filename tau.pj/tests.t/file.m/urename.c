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

enum { MAX_NAME = 255 };

char	NameA[MAX_NAME];
char	NameB[MAX_NAME];
char	NameC[MAX_NAME];

void make_names (char *name)
{
	int	fd;

	cat(NameA, name, "_A");
	cat(NameB, name, "_B");
	cat(NameC, name, "_C");
	fd = open(NameA, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	close(fd);
}

void usage (char *name)
{
	fprintf(stderr, "%s <base_file_name> <num_iterations>\n", name);
	exit(1);
}

int main (int argc, char *argv[])
{
	char		*name = "";
	int		rc;
	unsigned	i;
	unsigned	n = 1000;

	if (argc < 2) {
		usage(argv[0]);
	}
	if (argc > 1) {
		name = argv[1];
	}
	if (argc > 2) {
		n = atoi(argv[2]);
	}
	make_names(name);
	for (;;) {
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
