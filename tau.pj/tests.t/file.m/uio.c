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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/fs.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <eprintf.h>
#include <debug.h>

#include <style.h>
#include <mystdlib.h>
#include <timer.h>
#include <puny.h>
#include <eprintf.h>

enum { BLK_SIZE = 1<<12 };

void usage (void)
{
	pr_usage("-f<file name> -i<iterations>");
}

off_t Range (off_t max)
{
	return random() % max;
}

int setup_file (char *file_name, off_t *num_blks)
{
	struct stat	sb;
	mode_t		mode;
	int		fd;
	ssize_t		rc;

	fd = open(file_name, O_RDONLY);
	if (fd == -1) {
		eprintf("open %s:", file_name);
	}
	rc = fstat(fd, &sb);
	if (rc == -1) {
		eprintf("fstat %s:", file_name);
	}
	mode = sb.st_mode;
	if (S_ISREG(mode)) {
		*num_blks = sb.st_size / BLK_SIZE;
#ifdef __linux__
	} else if (S_ISBLK(mode)) {
		off_t	size;

		rc = ioctl(fd, BLKGETSIZE64, &size);
		if (rc == -1) {
			eprintf("ioctl %s:", file_name);
		}
		*num_blks = size / BLK_SIZE;
#endif
	} else {
		eprintf("don't know type(%x) of file %s", mode, file_name);
	}
	printf("num_blks=%lld\n", (u64)*num_blks);
	return fd;
}

char Buf[BLK_SIZE];
cascade_s	Pread;

int main (int argc, char *argv[])
{
	char		*file = "xyzzy";
	unsigned	cnt = 1000000;
	int		fd;
	unsigned	i;
	off_t		offset;
	off_t		num_blks;
	u64		start;
	u64		end;
	ssize_t		rc;

	punyopt(argc, argv, NULL, NULL);
	file = Option.file;
	cnt  = Option.iterations;
	seed_random();
	fd = setup_file(file, &num_blks);
	if (num_blks == 0) {
		fatal("file %s is too small for test", file);
	}
	for (i = 0; i < cnt; i++) {
		offset = Range(num_blks);
		start = nsecs();
		rc = pread(fd, Buf, BLK_SIZE, offset);
		end = nsecs();
		if (rc == -1) {
			eprintf("pread failed at offset %ld\n", offset);
		}
		if (rc != BLK_SIZE) {
			eprintf("pread only read %lld bytes.", rc);
		}
		cascade(Pread, end - start);
	}
	pr_cascade("pread", Pread);
	return 0;
}
