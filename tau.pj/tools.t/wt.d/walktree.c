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

typedef struct Ftw_s {
	u64	files;
	u64	dirs;
	u64	dir_no_read;
	u64	no_stat;
	u64	dir_path;
	u64	symlink;
	u64	broken_symlink;
} Ftw_s;

typedef struct Stat_s {
	u64	gt1link;
} Stat_s;

Ftw_s Ftw;
Stat_s Stat;

void pr_ftw_flag(const char *dir)
{
	printf("%s:\n"
		"\tfiles=%lld dirs=%lld dir_no_read=%lld no_stat=%lld"
		" dir_path=%lld symlink=%lld broken_symlink=%lld total=%lld\n",
		dir,
		Ftw.files, Ftw.dirs, Ftw.dir_no_read, Ftw.no_stat,
		Ftw.dir_path, Ftw.symlink, Ftw.broken_symlink,
		Ftw.files + Ftw.dirs + Ftw.dir_no_read + Ftw.no_stat
		+ Ftw.dir_path + Ftw.symlink + Ftw.broken_symlink);
}

void pr_stat(void)
{
	printf("hardlinks > 1 = %lld\n", Stat.gt1link);
}

int do_entry(
	const char *fpath,
	const struct stat *sb,
	int typeflag,
	struct FTW *ftwbuf)
{
//	printf("%s\n", fpath);
	switch (typeflag) {
	case FTW_F:
		++Ftw.files;
		if (sb->st_nlink > 2) {
			++Stat.gt1link;
			printf("%s\n", fpath);
		}
		break;
	case FTW_D:
		++Ftw.dirs;
		break;
	case FTW_DNR:
		++Ftw.dir_no_read;
		break;
	case FTW_NS:
		++Ftw.no_stat;
		break;
	case FTW_DP:
		++Ftw.dir_path;
		break;
	case FTW_SL:
		++Ftw.symlink;
		break;
	case FTW_SLN:
		++Ftw.broken_symlink;
		break;
	default:
		warn("Don't know file type %s=%d", fpath, typeflag);
		break;
	}
	return 0;
}

void nftw_walk_tree(char *dir)
{
	nftw(dir, do_entry, 200, FTW_CHDIR | FTW_PHYS /*| FTW_MOUNT*/);
}

int main (int argc, char *argv[])
{
	char *dir = ".";
	tick_t start;
	tick_t finish;
	tick_t total;

	if (argc > 1) dir = argv[1];

	start = nsecs();
	nftw_walk_tree(dir);
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs %g secs\n", total, (double)total/1000000000.0);

	pr_ftw_flag(dir);
	pr_stat();
	return 0;
}
