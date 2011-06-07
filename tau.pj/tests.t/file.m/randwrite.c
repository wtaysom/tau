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
#include <crc.h>
#include <puny.h>
#include "randrw.h"

Buf_s	Buf;

void initBuf (void)
{
	int		i;

	Buf.seed = random();
	srandom(Buf.seed);

	for (i = 0; i < NUM_LONGS; ++i)
	{
		Buf.data[i] = random();
	}
	Buf.crc = crc32((unsigned char *)Buf.data, sizeof(Buf.data));
}

int main (int argc, char *argv[])
{
	int		fd;
	char	*name;
	off_t	size;
	off_t	max_size;
	ssize_t	bytes;

	punyopt(argc, argv, NULL, NULL);
	name = Option.file;
	max_size = Option.file_size;
	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	for (size = 0; size < max_size; ++size) {
		initBuf();
		bytes = write(fd, &Buf, sizeof(Buf));
		if (bytes == -1) {
			perror("write");
			exit(1);
		}
	}
	close(fd);
	return 0;
}
