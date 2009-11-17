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
 * Remove a directory tree.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/errno.h>

#include <myio.h>

static void error (const char *what, const char *who)
{
	if (what && who) {
		fprintf(stderr, "ERROR: %s: %s: %s\n",
			what, who, strerror(errno));
	} else if (what) {
		fprintf(stderr, "ERROR: %s: %s\n", what, strerror(errno));
	} else {
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
	}
	exit(1);
}

void rmlevel (void)
{
	DIR		*dir;
	struct dirent	*entry;
	char		name[255];

	dir = opendir(".");
	if (!dir) {
		error("opendir", ".");
	}
	for (;;) {
		entry = readdir(dir);
		if (entry == NULL) {
			break;
		}
printf("%s\n", entry->d_name);
		if (strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		unlink(name);
		strcpy(name, entry->d_name);
		#if 0
		if (chdir(entry->d_name)) {
			unlinkq(entry->d_name);
		} else {
			rmlevel();
			chdirq("..");
			rmdirq(entry->d_name);
		}
		#endif
	}
	closedir(dir);
}

void rmtreeq (const char *path)
{
	int	current;

	current = openq(".", O_RDONLY);
	chdir(path);

	rmlevel();

	fchdir(current);
	rmdirq(path);
}

void usage (char *name)
{
	fprintf(stderr, "%s <directory>\n",
		name);
	exit(1);
}

int main (int argc, char *argv[])
{
	char		*directory = "";

	if (argc < 2) {
		usage(argv[0]);
	}
	if (argc > 1) {
		directory = argv[1];
	}
	rmtreeq(directory);
	return 0;
}
