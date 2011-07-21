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

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <style.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <myio.h>
#include <puny.h>

char FileTypes[] = {	'0', 'p', 'c', '3', 'd', '5', 'b', '7',
			 '-', '9', 'l', 'B', 's', 'D', 'E', 'F' };

void prStatNice (struct stat *sb)
{
	char	*time = date(sb->st_atime);

	printf("%c%.3o %9qu %s",
		FileTypes[(sb->st_mode >> 12) & 017],
		sb->st_mode & 0777,
		(u64)sb->st_size,
		time);
	printf(" %s", date(sb->st_mtime));
}

void mystat (int fd, char *msg)
{
	struct stat	sb;

	fstatq(fd, &sb);
	prStatNice( &sb);
	printf(" %s\n", msg);
}

void usage (void)
{
	pr_usage("-f<file>");
}

char	Write[] = "Hello, my name is Charlie\n";
char	Read[1024];
char	Test[] = "test";

int main (int argc, char *argv[])
{
	char		*file;
	int		fd;

	punyopt(argc, argv, NULL, NULL);
	file = Option.file;

	fd = openq(file, O_RDWR | O_CREAT);
	mystat(fd, "after open/create");
	sleep(2);
	writeq(fd, Write, sizeof(Write));
	mystat(fd, "after write");
	closeq(fd);


	sleep(2);
	fd = openq(file, O_RDWR | O_NOATIME);
	sleep(2);
	mystat(fd, "after open noatime");
	sleep(2);
	mystat(fd, "before read noatime");
	readq(fd, Read, sizeof(Read));
	mystat(fd, "after read noatime");
	closeq(fd);

	sleep(2);
	fd = openq(file, O_RDWR);
	mystat(fd, "after open rdwr");
	sleep(2);
	mystat(fd, "before read");
	readq(fd, Read, sizeof(Read));
	mystat(fd, "after read");
	closeq(fd);

	sleep(2);
	fd = openq(file, O_RDWR);
	mystat(fd, "after open");
	sleep(2);
	fsetxattr(fd, "noatime", Test, sizeof(Test), 0);
	mystat(fd, "after setxattr");
	sleep(2);
	mystat(fd, "before read");
	readq(fd, Read, sizeof(Read));
	mystat(fd, "after read");
	sleep(2);
	fgetxattr(fd, "noatime", Read, sizeof(Read));
	mystat(fd, "after getxattr");
	closeq(fd);

	return 0;
}
