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
#include <puny.h>
#include <eprintf.h>

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

	dir = opendir(".");
	if (!dir) {
		error("opendir", ".");
	}
	for (;;) {
		entry = readdir(dir);
		if (entry == NULL) {
			break;
		}
		if (strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		if (chdir(entry->d_name) == -1) {
			unlinkq(entry->d_name);
		} else {
			rmlevel();
			chdirq("..");
			rmdirq(entry->d_name);
		}
	}
	closedir(dir);
}

void rmtreeq (const char *path)
{
	int	current;

	current = openq(".", O_RDONLY);
	if (chdir(path)) perror("chdir");

	rmlevel();

	if (fchdir(current)) perror("fchdir");
	rmdirq(path);
}

void usage (void)
{
	pr_usage("-d<directory>");
}

int main (int argc, char *argv[])
{
	punyopt(argc, argv, NULL, NULL);
	rmtreeq(Option.dir);
	return 0;
}
