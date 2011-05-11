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

#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <puny.h>

enum {	MAX_FILE  = 1ULL << 41,
	FILE_MASK = MAX_FILE - 1,
	BIG_FILE  = 1ULL << 40,
	BLK_SIZE  = 4096 };

void prompt (char *s)
{
	static int	cnt = 0;

	++cnt;
	printf("%s:%s %d> ", getprogname(), s, cnt);
	getchar();
}

void fill (void *buf, int size)
{
	char	*b = buf;
	int	i;

	for (i = 0; i < size; i++) {
		*b++ = random();
	}
}

void write_sparse (int fd, off_t seek)
{
	char	buf[BLK_SIZE];
	off_t	new;
	int	rc;

	new = lseek(fd, seek, 0);
	if (new == -1) {
		eprintf("lseek of %lld failed: ", seek);
	}
	fill(buf, sizeof(buf));
	rc = write(fd, buf, sizeof(buf));
	if (rc == -1) {
		eprintf("write failed: ");
	}
}

void make_sparse (int fd)
{
	write_sparse(fd, BIG_FILE);
}

void write_mmap (int fd, off_t seek)
{
	void	*buf;
	int	page_size;
	int	rc;

	page_size = getpagesize();
	buf = mmap(NULL, page_size, PROT_WRITE, MAP_SHARED, fd, seek);
	if (buf == (void *)-1) {
		eprintf("mmap seek=%lld :", seek);
	}
	fill(buf, page_size);
	rc = munmap(buf, page_size);
	if (rc == -1) {
		eprintf("munmap buf=%p pos%lld :", buf, seek);
	}
}

void read_mmap (int fd, off_t seek)
{
	void	*buf;
	int	page_size;
	int	rc;

	page_size = getpagesize();
	buf = mmap(NULL, page_size, PROT_READ, MAP_SHARED, fd, seek);
	if (buf == (void *)-1) {
		eprintf("mmap seek=%lld :", seek);
	}
prmem("buf", buf, page_size);
	rc = munmap(buf, page_size);
	if (rc == -1) {
		eprintf("munmap buf=%p pos%lld :", buf, seek);
	}
}

void sparse_file (int fd)
{
	void	*buf;
	off_t	pos;
	int	page_size;
	int	rc;

	page_size = getpagesize();
	for (pos = page_size; pos < MAX_FILE; pos <<= 1) {
		buf = mmap(NULL, page_size, PROT_READ|PROT_WRITE,
				MAP_SHARED, fd, pos);
PRd(pos);
PRp(buf);
prompt("prmem");
prmem("buf", buf, page_size);
//		fill(buf, page_size);
		rc = munmap(buf, page_size);
		if (rc == -1) {
			eprintf("munmap buf=%p pos%lld :", buf, pos);
		}
	}
}

void t1 (int fd)
{
	make_sparse(fd);
	write_mmap(fd, BIG_FILE >> 1);
}

void t2 (int fd)
{
	make_sparse(fd);
	read_mmap(fd, BIG_FILE >> 1);
}

void usage (void)
{
	pr_usage("-f<file>");
}

int main (int argc, char *argv[])
{
	char	*file = "sparse_file";
	int	fd;

	punyopt(argc, argv, NULL, NULL);
	file = Option.file;

	fd = open(file, O_CREAT|O_RDWR/*|O_TRUNC|O_DIRECT*/, 0666);
	if (fd == -1) {
		eprintf("open %s:", file);
	}
	t2(fd);
	close(fd);
//	unlink(file);

	return 0;
}
