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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <puny.h>

/* Quick test for monster seek followed by a write */
int main (int argc, char *argv[])
{
	int	fd;
	int	rc;

	punyopt(argc, argv, NULL, NULL);
	fd = open(Option.file, O_RDWR | O_CREAT | O_TRUNC, 0666);
	rc = lseek(fd, 0x1000000000LL, 0);
	if (rc == -1) {
		perror("lseek");
		exit(2);
	}
	rc = write(fd, "hi", 2);
	if (rc == -1) {
		perror("write");
		exit(2);
	}
	return 0;
}
