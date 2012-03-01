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
 * Random write microbenchmark. Use synchronous writes because writes
 * could be reordered if just written to a buffer. This will be more
 * like database updates.
 */

#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <puny.h>

u64 Bufsize_log2 = 12;

void usage (void)
{
	pr_usage("-f<file_name> -z<file_size> -i<num_iterations>"
		" -b<bufsize_log2> -l<loops>");
}

bool myopt (int c)
{
	switch (c) {
	case 'b':
		Bufsize_log2 = strtoll(optarg, NULL, 0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int main (int argc, char *argv[])
{
	u8		*buf;
	int		fd;
	unsigned	i;
	unsigned	bufsize;
	unsigned	n;
	u64		size;
	u64		numbufs;
	u64		offset;
	u64		l;
	ssize_t		rc;

	punyopt(argc, argv, myopt, "b:");
	n = Option.iterations;
	size = Option.file_size;
	bufsize = 1 << Bufsize_log2;
	buf = emalloc(bufsize);
	numbufs = size / bufsize;

	fd = open(Option.file, O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0666);
	for (i = 0; i < bufsize; ++i) {
		buf[i] = random();
	}
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			offset = urand(numbufs) * bufsize;
			/* Make buffer unique */
			buf[urand(bufsize)] = random();
			rc = pwrite(fd, buf, bufsize, offset);
			if (rc != bufsize) {
				if (rc == -1) fatal("pwrite:");
				fatal("pwrite rc=%d offset=%lld", rc, offset);
			}
		}
		stopTimer();
		printf("size=%lld n=%d ", size, n);
		prTimer();

		printf("\t%6.4g MiB/s",
			(double)(n * bufsize) / get_avg() / MEBI);

		printf("\t%6.4g IOPs/sec",
			(double)(n) / get_avg());

		printf("\n");
	}
	close(fd);
	unlink(Option.file);
	return 0;
}
