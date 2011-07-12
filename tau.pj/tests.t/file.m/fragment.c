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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <style.h>
#include <mylib.h>
#include <eprintf.h>

struct {
	u64	num_files;
} Inst;

char	Buf[4096];

static void init_buf (void)
{
	static char	rnd_char[] = "abcdefghijklmnopqrstuvwxyz\n";
	int		i;

	for (i = 0; i <  sizeof(Buf); i++) {
		Buf[i] = rnd_char[urand(sizeof(rnd_char)-1)];
	}
}

static int fill (int fd, u64 size)
{
	int	n;
	int	rc;

	for (n = sizeof(Buf); size; size -= n) {
		if (n > size) {
			n = size;
		}
		rc = write(fd, Buf, n);
		if (rc == -1) {
			if (errno == ENOSPC) {
				return errno;
			}
			return -1;
		}
	}
	return 0;
}

static int create_file (char *name, u64 size)
{
	int	fd;
	int	rc;

	fd = creat(name, 0666);
	if (fd == -1) {
		if (errno == ENOSPC) {
			return errno;
		}
		eprintf("creat \"%s\" :", name);
		return -1;
	}
	++Inst.num_files;
	rc = fill(fd, size);
	if (rc) {
		if (errno == ENOSPC) {
			return errno;
		}
		eprintf("fill \"%s\" :", name);
	}
	close(fd);
	return 0;
}

int main (int argc, char *argv[])
{
	char	name[16];
	int	i;
	int	rc;

	init_buf();

	for (i = 0; ; i++) {
		snprintf(name, sizeof(name)-1, "%u", i);
		rc = create_file(name, 1);
		if (rc) break;
	}
	if (rc != ENOSPC) {
		perror("Didn't run out of space");
		return 2;
	}
	for (i = 0; ; i += 2) {
		snprintf(name, sizeof(name)-1, "%u", i);
		rc = unlink(name);
		if (rc) break;
	}
	return 0;
}
