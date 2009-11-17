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

#define _GNU_SOURCE		1
#define _LARGEFILE64_SOURCE	1
#define _FILE_OFFSET_BITS	64

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dumpvalue (int fd, char *name)
{
	unsigned char	value[1<<16];
	ssize_t		size;
	unsigned	i;

	size = fgetxattr(fd, name, value, sizeof(value));
	if (size == -1) {
		perror("getxattr");
		exit(1);
	}
	for (i = 0; i < size; i++) {
		if (!(i % 16)) {
			printf("\n");
		}
		printf(" %2x", value[i]);
	}
	printf("\n");
}

void dumpxattr (char *path)
{
	int	fd;
	ssize_t	size;
	char	*name;
	char	list[1<<16];

	fd = open(path, O_RDONLY | O_NOATIME);
	if (fd == -1) {
		perror(path);
		exit(1);
	}
	size = flistxattr(fd, list, sizeof(list));
	if (size == -1) {
		perror("flistxatrr");
		exit(1);
	}
	name = list;
	while (name - list < size) {

		printf("%s", name);
		dumpvalue(fd, name);

		name += strlen(name) + 1;
	}
	close(fd);
}

int main (int argc, char *argv[])
{
	unsigned	i;

	if (argc < 2) {
		printf("Usage: %s filename ...\n", argv[0]);
		exit(1);
	}

	for (i = 1; i < argc; i++) {
		dumpxattr(argv[i]);
	}
	return 0;
}
