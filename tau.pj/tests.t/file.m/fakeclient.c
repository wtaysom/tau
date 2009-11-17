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
 * WRITE FILES test -- write many files.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <mylib.h>
#include <myio.h>

#define BUF_SIZE	(1<<16)

int	Buf[BUF_SIZE];

void write_test (char *name, u64 size)
{
	int		fd;
	ssize_t		written;
	size_t		toWrite;
	u64		rest;

	fd = openq(name, O_RDWR | O_CREAT | O_TRUNC);
	for (rest = range(size); rest; rest -= written) {
		if (rest > BUF_SIZE) {
			toWrite = BUF_SIZE;
		} else {
			toWrite = rest;
		}
		written = write(fd, Buf, toWrite);
		if (written != toWrite) {
			if (written == -1) {
				perror("write");
				exit(1);
			}
			fprintf(stderr,
				"toWrite=%llu != written=%lld\n",
				(u64)toWrite, (s64)written);
			exit(1);
		}
	}
//	fsync(fd);
	closeq(fd);
}

void usage (char *name)
{
	fprintf(stderr,
		"%s <directory> [<file_size> [<num_iterations> [<sleep]]]\n",
		name);
	exit(1);
}

int main (int argc, char *argv[])
{
	char		*directory = ".";
	char		name[256];
	unsigned	i, j;
	unsigned	n = 1000;
	unsigned	seconds = 30;
	u64		size = (1<<19);

	seed_random();

	if (argc < 2) {
		usage(argv[0]);
	}
	if (argc > 1) {
		directory = argv[1];
	}
	if (argc > 2) {
		size = atoll(argv[2]);
	}
	if (argc > 3) {
		n = atoi(argv[3]);
	}
	if (argc > 4) {
		seconds = atoi(argv[4]);
	}
	for (i = 0; i < BUF_SIZE; ++i)
	{
		Buf[i] = random();
	}
	mkdirq(directory);
	chdirq(directory);
	for (j = 0; ; ++j) {
		sprintf(name, "dir_%d", j);
		mkdirq(name);
		chdirq(name);
		for (i = 0; i < n; ++i) {
			sprintf(name, "file_%d", i);
			startTimer();
			write_test(name, size);
			stopTimer();
			sleep(seconds);
		}
		prTimer();
		printf(" size=%lld n=%d\n", size, n);
		chdir("..");
	}
}
