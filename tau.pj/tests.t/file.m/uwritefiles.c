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

#define BUF_SIZE	(1<<16)

int	Buf[BUF_SIZE];

void write_test (char *name, u64 size)
{
	int		fd;
	ssize_t		written;
	size_t		toWrite;
	u64		rest;

	fd = openq(name, O_RDWR | O_CREAT | O_TRUNC);
	for (rest = size; rest; rest -= written) {
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
				"toWrite=%lu != written=%ld\n",
				(unint)toWrite, (snint)written);
			exit(1);
		}
	}
//	fsync(fd);
	closeq(fd);
}

void usage (void)
{
	pr_usage("-d<directory> -z<file_size> -i<num_iterations> -l<loops>");
}

int main (int argc, char *argv[])
{
	char	*directory = "";
	char	name[256];
	unint	i, j;
	unint	n = 1000;
	u64	size = (1<<20);

	punyopt(argc, argv, NULL, NULL);
	directory = Option.dir;
	size = Option.file_size;
	n = Option.iterations;
	for (i = 0; i < BUF_SIZE; i++)
	{
		Buf[i] = random();
	}
	mkdir(directory, 0777);
	chdirq(directory);
	for (j = 0; j < Option.loops; j++) {
		sprintf(name, "dir_%ld", j);
		mkdirq(name);
		chdirq(name);
		startTimer();
		for (i = 0; i < n; i++) {
			sprintf(name, "file_%ld", i);
			write_test(name, size);
		}
		stopTimer();
		prTimer();
		printf(" size=%lld n=%ld\n", size, n);
		if (chdir("..")) perror("chdir ..");
	}
	return 0;
}
