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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <puny.h>

char	Data[] = "This is a test\n";
char	Buf[sizeof(Data)];

int main (int argc, char *argv[])
{
	int	fd;
	int	rc;
	char	*name;
	ssize_t	bytes;

	punyopt(argc, argv, NULL, NULL);
	name = Option.file;

	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	//rc = lseek(fd, 1000000000, 0);
	rc = lseek(fd, 0x800000000LL, 0);
	if (rc == -1) {
		rc = errno;
		perror("lseek before write");
		printf("errno=%d\n", errno);
		exit(1);
	}
	bytes = write(fd, Data, sizeof(Data));
	if (bytes == -1) {
		perror("write");
		exit(1);
	}
	//lseek(fd, 0, 0);
	lseek(fd, 0x800000000LL, 0);
	bytes = read(fd, Buf, sizeof(Buf));
	if (rc == -1) {
		perror("lseek before write");
		exit(1);
	}
	if (bytes == -1) {
		perror("read");
		exit(1);
	}
	close(fd);
//	rc = unlink(name);
	return 0;
}
