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
#include <puny.h>

void usage (void)
{
	pr_usage("-f<file_name> -z<file_size> -i<num_iterations> -b<bufsize_log2>");
}

int Bufsize_log2 = 12;

void myopt (int c)
{
	switch (c) {
	case 'b':
		Bufsize_log2 = strtoll(optarg, NULL, 0);
		break;
	default:
		usage();
		break;
	}
}

int main (int argc, char *argv[])
{
	char	*name = "";
	u8	*buf;
	int	fd;
	ssize_t	written;
	size_t	toWrite;
	u32	i;
	u32	bufsize = (1<<12);
	u32	n = 1000;
	u64	size = (1<<20);
	u64	rest;
	u64	l;

	punyopt(argc, argv, myopt, "b:");
	name = Option.file;
	bufsize = (1 << Bufsize_log2);
	size = Option.file_size;
	n = Option.iterations;

	buf = emalloc(bufsize);
	for (i = 0; i < bufsize; ++i) {
		buf[i] = random();
	}
	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; i++) {
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
