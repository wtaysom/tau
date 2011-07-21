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
 * read microbenchmark
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

void fill_file (int fd, u64 size)
{
	ssize_t		written;
	size_t		toWrite;
	unsigned	i;
	u64		rest;
	u8		buf[1<<12];

	for (i = 0; i < sizeof(buf); ++i)
	{
		buf[i] = random();
	}
	for (rest = size; rest; rest -= written) {
		if (rest > sizeof(buf)) {
			toWrite = sizeof(buf);
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
				"toWrite=%llu != written=%lld\n",
				(u64)toWrite, (s64)written);
			exit(1);
		}
	}
	fsync(fd);
	lseek(fd, 0, 0);
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
	ssize_t		haveRead;
	size_t		toRead;
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

	fd = open(Option.file, O_RDWR | O_CREAT | O_TRUNC, 0666);
	fill_file(fd, size);
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (i = 0; i < n; ++i) {
			for (rest = size; rest; rest -= haveRead) {
				if (rest > bufsize) {
					toRead = bufsize;
				} else {
					toRead = rest;
				}
				haveRead = read(fd, buf, toRead);
				if (haveRead != toRead) {
					if (haveRead == -1) {
						perror("read");
						exit(1);
					}
					fprintf(stderr,
						"toRead=%llu != haveRead=%lld\n",
						(u64)toRead, (s64)haveRead);
					exit(1);
				}
			}
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
