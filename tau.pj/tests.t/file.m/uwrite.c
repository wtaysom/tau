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
#include <mylib.h>
#include <eprintf.h>

void usage (char *name)
{
	fprintf(stderr,
		"%s <file_name> <file_size> <num_iterations> <bufsize_log2>\n",
		name);
	exit(1);
}

int main (int argc, char *argv[])
{
	char		*name = "";
	u8		*buf;
	int		fd;
	ssize_t		written;
	size_t		toWrite;
	unsigned	i;
	unsigned	bufsize = (1<<12);
	unsigned	n = 1000;
	u64		size = (1<<20);
	u64		rest;

	if (argc < 2) {
		usage(argv[0]);
	}
	if (argc > 1) {
		name = argv[1];
	}
	if (argc > 2) {
		size = atoll(argv[2]);
	}
	if (argc > 3) {
		n = atoi(argv[3]);
	}
	if (argc > 4) {
		bufsize = 1 << atoi(argv[4]);
	}
	buf = emalloc(bufsize);
	for (i = 0; i < bufsize; ++i)
	{
		buf[i] = random();
	}
	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	for (;;) {
		startTimer();
		for (i = 0; i < n; ++i) {
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
			}
			//fsync(fd);
			lseek(fd, 0, 0);
		}
		stopTimer();
		printf("size=%lld n=%d ", size, n);
		prTimer();
		printf("\n");
	}
	close(fd);
	return 0;
}
