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

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <eprintf.h>
#include <puny.h>

int main (int argc, char *argv[])
{
	int	fd;
	int	rc;

	punyopt(argc, argv, NULL, NULL);
	fd = open(Option.file, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		eprintf("Couldn't create %s:", Option.file);
	}
	rc = fcntl(fd, F_SETLEASE, F_WRLCK);
	if (rc == -1) {
		eprintf("parent fcntl:");
	}
	getchar();
	return 0;
}
