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
#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdio.h>

#include <eprintf.h>
#include <myio.h>
#include <timer.h>

int NumFiles;

void prfile (char *path, void *arg)
{
//	printf("%s\n", path);
	++NumFiles;
}

void donothing(char *path, void *arg, int level)
{
	struct stat sb;

	++NumFiles;
	int rc = stat(path, &sb);
	if (rc) warn("%s", path);
}

int do_entry(
	const char *fpath,
	const struct stat *sb,
	int typeflags,
	struct FTW *ftwbuf)
{
	++NumFiles;
	return 0;
}

void nftw_walk_tree(char *dir)
{
	nftw(dir, do_entry, 200, FTW_CHDIR | FTW_PHYS);
}

int main (int argc, char *argv[])
{
	char *dir = ".";
	tick_t start;
	tick_t finish;
	tick_t total;

	if (argc > 1) dir = argv[1];

	NumFiles = 0;
	start = nsecs();
	nftw_walk_tree(dir);
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs %d\n", total, NumFiles);

	NumFiles = 0;
	start = nsecs();
	walk_dir(dir, donothing, NULL, 0);
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs %d\n", total, NumFiles);

	NumFiles = 0;
	start = nsecs();
	walk_tree(dir, prfile, NULL);
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs %d\n", total, NumFiles);

	return 0;
}
