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
#include <string.h>

#include <eprintf.h>
#include <puny.h>

char	Data[] = "This is a test";
char	Buf[sizeof(Data)];

void prompt (char *m)
{
	printf("%s", m);
	getchar();
}

int main (int argc, char *argv[])
{
	int	fd;
	int	rc;
	char	*name;
	ssize_t	bytes;

	punyopt(argc, argv, NULL, NULL);
	name = Option.file;

	prompt("open");
	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) eprintf("open %s:", name);

	prompt("write");
	bytes = write(fd, Data, sizeof(Data));
	if (bytes == -1) eprintf("write %s:", name);

	prompt("lseek");
	rc = lseek(fd, 0, 0);
	if (rc == -1) eprintf("lseek %s:", name);

	prompt("read");
	bytes = read(fd, Buf, sizeof(Buf));
	if (bytes == -1) eprintf("read %s:", name);

	if (bytes != sizeof(Buf)) eprintf("only read %d bytes of %d",
						bytes, sizeof(Buf));

	if (strcmp(Data, Buf) != 0) eprintf("didn't read what I wrote %s!=%s",
						Data, Buf);

	prompt("close");
	close(fd);
	return 0;
}
