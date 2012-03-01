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
 * write microbenchmark
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

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
	ssize_t		written;
	size_t		toWrite;
	unsigned	i;
	unsigned	bufsize;
	unsigned	n;
	u64		size;
	u64		rest;
	u64		l;

	punyopt(argc, argv, myopt, "b:");
	n = Option.iterations;
	size = Option.file_size;
	bufsize = 1 << Bufsize_log2;

	buf = emalloc(bufsize);
	for (i = 0; i < bufsize; ++i) {
		buf[i] = random();
	}
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; i++) {
			/* Because eCryptfs has to decrypt a page before
			 * overwriting it, recreating the file on each
			 * iteration gives a more realistic value.
			 */
			fd = open(Option.file,
				O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (fd == -1) fatal("open %s:", Option.file);
			for (rest = size; rest; rest -= written) {
				if (rest > bufsize) {
					toWrite = bufsize;
				} else {
					toWrite = rest;
				}
				written = write(fd, buf, toWrite);
				if (written != toWrite) {
					if (written == -1) {
						perror("write");
						exit(1);
					}
					fprintf(stderr,
						"toWrite=%lu != written=%ld\n",
						(unint)toWrite, (snint)written);
					exit(1);
				}
				/* Make next buffer unique */
				buf[urand(bufsize)] = random();
			}
			fsync(fd);
			close(fd);
		}
		stopTimer();
		printf("size=%lld n=%d ", size, n);
		prTimer();
		printf("\n");
	}
	unlink(Option.file);
	return 0;
}
