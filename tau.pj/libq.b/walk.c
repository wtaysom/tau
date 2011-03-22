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

/* Walk a directory tree */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <myio.h>

void walk_dir (char *name, dir_f f, void *arg, int level)
{
	DIR				*dir;
	struct dirent	*de;

	f(name, arg, level);

	if (chdir(name) == -1) return;
	dir = opendir(".");
	if (dir == NULL) return;


	for (;;) {
		de = readdir(dir);
		if (!de) break;
		if (strcmp(de->d_name, ".") == 0) continue;
		if (strcmp(de->d_name, "..") == 0) continue;
		walk_dir(de->d_name, f, arg, level+1);
	}
	closedir(dir);
	if (chdir("..")) return;
}

#if 0
void prlevel (int level)
{
	int	i;

	for (i = 0; i < level; i++) {
		printf("  ");
	}
}

void pr_dir (char *name, void *arg, int level)
{
	prlevel(level);
	printf("%s\n", name);
}

int main (int argc, char *argv[])
{
	walk_dir(argv[1], pr_dir, NULL, 0);
	return 0;
}
#endif
