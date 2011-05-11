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

#include <style.h>
#include <crc.h>
#include <histogram.h>
#include <puny.h>
#include "randrw.h"

Buf_s	Buf;
	
bool checkBuf (void)
{
	int	i;
	long	x;

	if (Buf.crc == crc32((unsigned char *)Buf.data, sizeof(Buf.data)))
	{
		return TRUE;
	}
	printf("crc failed\n");
	srandom(Buf.seed);
	for (i = 0; i < NUM_LONGS; ++i)
	{
		x = random();
		if (Buf.data[i] != x)
		{
			printf("%d. %lx!=%lx\n", i, Buf.data[i], x);
		}
	}
	return FALSE;
}

int main (int argc, char *argv[])
{
	int	fd;
	char	*name;
	off_t	size;
	off_t	max_size;
	ssize_t	bytes;

	punyopt(argc, argv, NULL, NULL);
	name = Option.file;
	max_size = Option.file_size;
	fd = open(name, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	for (size = 0; size < max_size; ++size) {
		bytes = read(fd, &Buf, sizeof(Buf));
		if (bytes == -1) {
			perror("read");
			exit(1);
		}
		if (!checkBuf())
		{
			printf("check failed %llu\n", (u64)size);
			exit(2);
		}
	}
	close(fd);
	return 0;
}
