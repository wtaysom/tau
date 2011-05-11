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
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include <style.h>
#include <puny.h>

/*
 * turning on htrees:  tune2fs -O dir_index /dev/what_ever_device
 */

enum { MAX_NAME = 20 };

static long range (long domain)
{
	return random() % domain;
}

char *gen_name (void)
{
	static char file_name_char[] =	"abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";
	static char	name[MAX_NAME + 2];
	char		*c;
	unsigned	len;

	len = MAX_NAME;
	for (c = name; len; c++, len--) {
		*c = file_name_char[range(sizeof(file_name_char) - 1)];
	}
	*c = '\0';
	return name;
}

void build_dir (int n)
{
	int	i;
	int	fd;
	char	*name;

	for (i = 0; i < n; i++) {
		name = gen_name();
		fd = open(gen_name(), O_RDWR | O_CREAT, 0666);
		if (fd == -1) {
			perror(name);
		} else {
			close(fd);
		}
	}
}

void tst_telldir (void)
{
	struct dirent	*de;
	struct dirent	old_de;
	DIR	*dir;
	off_t	offset;
	off_t	old_offset;

	old_offset = 0;
	dir = opendir(".");
	if (!dir) {
		perror("\".\"");
	}
	for (;;) {
		de = readdir(dir);
		if (!de) break;
		offset = telldir(dir);
		if (offset == old_offset) {
			printf("%llx %llx %s\n",
				(u64)old_de.d_off, (u64)offset, old_de.d_name);
			printf("%llx %llx %s\n\n",
				(u64)de->d_off, (u64)offset, de->d_name);
		}
		old_offset = offset;
		old_de = *de;
	}
	closedir(dir);
}

int main (int argc, char *argv[])
{
	char	*dir;
	int	rc;

	punyopt(argc, argv, NULL, NULL);
	dir = Option.dir;
	rc = chdir(dir);
	if (rc) {
		perror(dir);
		exit(2);
	}
	build_dir(300000);
	tst_telldir();
	return 0;
}
