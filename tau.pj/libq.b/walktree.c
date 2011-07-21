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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <debug.h>
#include <style.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <myio.h>


enum { MAX_PATH = 4096 };

struct {
	u64	num_dirs;
	u64	num_files;
} Inst;

void pr_inst (void)
{
	printf("dirs    = %10llu\n", Inst.num_dirs);
	printf("files   = %10llu\n", Inst.num_files);
}

static void add_name (char *parent, char *child)
{
	char	*c;

	c = &parent[strlen(parent)];
	cat(c, "/", child, NULL);
}

static void drop_name (char *path)
{
	char	*c;

	for (c = &path[strlen(path)]; c != path; c--) {
		if (*c == '/') {
			break;
		}
	}
	*c = '\0';
}

static int is_dir (const char *dir)
{
	struct stat	stbuf;
	int		rc;

	rc = stat(dir, &stbuf);
	if (rc) {
		eprintf("is_dir stat \"%s\" :", dir);
		return 0;
	}
	return S_ISDIR(stbuf.st_mode);
}

static DIR *open_dir (char *name)
{
	DIR	*dir;

	dir = opendir(name);
	if (!dir) {
		eprintf("open_dir \"%s\" :", name);
		return NULL;
	}
	++Inst.num_dirs;
	return dir;
}

static void close_dir (DIR *dir)
{
	closedir(dir);
}

#if 0
static void pr_indent (unsigned level)
{
	while (level--) {
		printf("  ");
	}
}
#endif

static void walkdir (char *path, file_f fn, void *arg, unsigned level)
{
	struct dirent	*d;
	DIR		*dir;

	dir = open_dir(path);
	for (;;) {
		d = readdir(dir);
		if (!d) break;
		if ((strcmp(d->d_name, ".")  == 0) ||
		    (strcmp(d->d_name, "..") == 0)) {
			continue;
		}
		//pr_indent(level); printf("%s\n", d->d_name);
		add_name(path, d->d_name);
		if (is_dir(path)) {
			walkdir(path, fn, arg, level+1);
		} else {
			fn(path, arg);
		}
		drop_name(path);
	}
	close_dir(dir);
}

void walk_tree (char *name, file_f fn, void *arg)
{
	char	path[MAX_PATH];

	strcpy(path, name);
	walkdir(path, fn, arg, 0);
}
