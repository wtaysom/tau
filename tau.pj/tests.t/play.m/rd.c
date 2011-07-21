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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/user.h>
#ifdef __APPLE__
#include <mach/vm_param.h>
#endif

#include <style.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <timer.h>

void fill (int fd, int pages)
{
	char	buf[4096];
	int	i;

	for (i = pages; i; i--) {
		if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
			perror("fill");
			exit(2);
		}
	}
	fsync(fd);
}

static void usage (void)
{
	fprintf(stderr, "Usage: %s [-b <num bytes to read>]"
			" [-p <num pages ot read>]"
			" [-s <size in pages of file>]"
			" [<num iterations> [<file>]]\n",
			getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	char	*file = "abc";
	char	*buf;
	u64	readsize = 1;
	u64	pages = 10;
	u64	n = 10000;
	u64	filesize;
	u64	remainder;
	int	i;
	int	fd;
	u64	startTime, endTime;
	u64	delta;

	setprogname(argv[0]);
	for (;;) {
		int	c;

		c = getopt(argc, argv, "b:p:s:?");
		if (c == -1) break;
		switch (c) {
		case 'b':
			readsize = strtoll(optarg, NULL, 0);
			break;
		case 'p':
			readsize = strtoll(optarg, NULL, 0) * PAGE_SIZE;
			break;
		case 's':
			pages = strtoll(optarg, NULL, 0);
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	if (argc > optind) {
		n = strtoll(argv[optind], NULL, 0);
	}
	if (argc > optind + 1) {
		file = argv[optind + 1];
	}
	filesize = pages * PAGE_SIZE;
	if (readsize > filesize) {
		eprintf("read size larger than file %llu > %llu",
			readsize, filesize);
	}
	buf = emalloc(readsize);

	fd = open("abc", O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (fd == -1) eprintf("open %s:", file);

	fill(fd, pages);
	lseek(fd, 0, 0);

	remainder = filesize;
	startTime = nsecs();
	for (i = n; i; i--) {
		if (read(fd, buf, readsize) == -1) {
			perror("read");
			exit(2);
		}
		remainder -= readsize;
		if (remainder < readsize) {
			remainder = filesize;
			lseek(fd, 0, 0);
		}
	}
	endTime = nsecs();
	delta = endTime - startTime;
	printf("diff=%lld nsecs per element=%g\n", delta, (double)delta/n);
	return 0;
}
