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

#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/fs.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <langinfo.h>
#include <locale.h>

#include <eprintf.h>
#include <debug.h>
#include <mystdlib.h>
#include <myio.h>
#include <timer.h>
#include <puny.h>

enum { BLK_SIZE = 1<<12 };

void usage (void)
{
	printf("Usage: %s [<file name> [<iterations>]]\n",
		getprogname());
}

enum { MAX_FILES = 1000, MAX_NAME = 8 };

typedef struct file_s {
	char	name[MAX_NAME];
	int	fd;
} file_s;

file_s	File[MAX_FILES];
file_s	*NextFile = File;

void gen_name (char *c)
{
	unsigned	i;
	static char file_name_char[] =  "abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";

	for (i = 0; i < MAX_NAME - 1; i++) {
		*c++ = file_name_char[urand(sizeof(file_name_char)-1)];
	}
	*c = '\0';
}

cascade_s	Creat;
cascade_s	Close;
cascade_s	Unlink;

void loop (unsigned cnt)
{
	unsigned	i;
	file_s		*f;

	for (i = 0; i < cnt; i++) {
		if (!(i & ((1<<13) - 1))) {
			printf(".");
			fflush(stdout);
		}
		if (percent(45)) {
			if (NextFile == &File[MAX_FILES]) continue;
			f = NextFile++;
			gen_name(f->name);
			TIME(Creat, f->fd = creat(f->name, 0700));
			if (f->fd == -1) {
				printf("name=%s\n", f->name);
				perror("creat");
				--NextFile;
			}
		} else {
			if (NextFile == File) continue;
			f = &File[urand(NextFile - File)];
			if (f->fd) {
				TIME(Close, close(f->fd));
				f->fd = 0;
			} else {
				TIME(Unlink, unlink(f->name));
				--NextFile;
				*f = *NextFile;
			}
		}
	}
	pr_cascade("creat", Creat);
	pr_cascade("close", Close);
	pr_cascade("unlink", Unlink);
}

int main (int argc, char *argv[])
{
	punyopt(argc, argv, NULL, NULL);
	chdirq(Option.dir);
	loop(Option.iterations);
	return 0;
}
