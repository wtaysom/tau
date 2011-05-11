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

#include <puny.h>

#define BUF_SIZE	(1<<16)

int	Buf[BUF_SIZE];

#define NUM_BUFS	(1<<16)	// (((1<<30) + (1<<29)) / sizeof(Buf))

int main (int argc, char *argv[])
{
	int	fd;
	ssize_t	bytes;
	long	i, j;

	punyopt(argc, argv, NULL, NULL);
	for (i = 0; i < BUF_SIZE; ++i) {
		Buf[i] = random();
	}
	fd = open(Option.file, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	for (j = 0; j < 1000000000; ++j) {
		for (i = 0; i < NUM_BUFS; ++i) {
			bytes = write(fd, Buf, sizeof(Buf));
			if (bytes == -1) {
				perror("write");
				fprintf(stderr, "buffers written %ld\n", i);
				exit(1);
			}
		}
		lseek(fd, 0, 0);
		printf(".");
		fflush(stdout);
	}
	close(fd);
	return 0;
}
