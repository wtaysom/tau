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
#include <puny.h>
#include <eprintf.h>
#include <debug.h>

#define BUF_SIZE	(1<<16)

int	Buf[BUF_SIZE];

void write_test (char *name, u64 size)
{
	int		fd;
	ssize_t		written;
	size_t		toWrite;
	u64		rest;

	fd = openq(name, O_RDWR | O_CREAT | O_TRUNC);
	for (rest = urand(size); rest; rest -= written) {
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

void usage (void)
{
	pr_usage("-d<directory> -z<file_size> -i<num_iterations>"
		" -s<sleep> -l<loops>");
}

int main (int argc, char *argv[])
{
	char		*directory;
	char		name[256];
	unsigned	i;
	unsigned	n;
	u64		size;
	u64		l;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	size = Option.file_size;
	directory = Option.dir;
	seed_random();

	for (i = 0; i < BUF_SIZE; ++i)
	{
		Buf[i] = random();
	}
	mkdirq(directory);
	chdirq(directory);
	for (l = 0; l < Option.loops; l++) {
		sprintf(name, "dir_%lld", l);
		mkdirq(name);
		chdirq(name);
		for (i = 0; i < n; ++i) {
			sprintf(name, "file_%d", i);
			startTimer();
			write_test(name, size);
			stopTimer();
			sleep(Option.sleep_secs);
		}
		prTimer();
		printf(" size=%lld n=%d\n", size, n);
		if (chdir("..")) perror("chdir ..");
	}
	return 0;
}
