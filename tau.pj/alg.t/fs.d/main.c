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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <style.h>
#include <fm.h>
#include <debug.h>
#include <mylib.h>
#include <eprintf.h>
#include <cmd.h>

#include <blk.h>
#include <b_inode.h>

/*
 * Work items:
 *	1. Delete
 *	2. More read/write tests
 *	3. ls with eof and mode
 *	4. Extents
 *	5. Alternating file creation
 */

enum { MAX_NAME = 20 /*128*/  };

char *gen_name (void)
{
	static char file_name_char[] =	"abcdefghijklmnopqrstuvwxyz"
					//"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";
	static char	name[MAX_NAME + 2];
	char		*c;
	unsigned	len;

	len = (range(MAX_NAME) + range(MAX_NAME) + range(MAX_NAME)) / 5 + 1;
	for (c = name; len; c++, len--) {
		*c = file_name_char[range(sizeof(file_name_char) - 1)];
	}
	*c = '\0';
	return name;
}

int qp (int argc, char *argv[])
{
	exit(0);
	return 0;
}

int genp (int argc, char *argv[])
{
	int	n;
	int	i;
	int	rc;
	char	*name;

	if (argc > 1) {
		n = atoi(argv[1]);
	} else {
		n = 10;
	}
	for (i = 0; i < n; i++) {
		name = gen_name();
		rc = Mknode(name, S_IFDIR);
		if (rc) {
			return rc;
		}
	}
	return 0;
}

int inodesp (int argc, char *argv[])
{
FN;
	dump_inodes();
	return 0;
}

int crp (int argc, char *argv[])
{
	int	i;
	int	rc;
FN;
	for (i = 1; i < argc; i++) {
		rc = Creat(argv[i]);
		if (rc) {
			printf("Couldn't ls %s %d\n", argv[i], rc);
			return rc;
		}
	}
	return 0;
}

int lsp (int argc, char *argv[])
{
	int	i;
	int	rc;
FN;
	if (argc < 2) {
		rc = Lsdir(NULL);
		if (rc) {
			printf("Couldn't ls root %d\n", rc);
		}
		return rc;
	}
	for (i = 1; i < argc; i++) {
		rc = Lsdir(argv[i]);
		if (rc) {
			printf("Couldn't ls %s %d\n", argv[i], rc);
			return rc;
		}
	}
	return 0;
}

int mkdirp (int argc, char *argv[])
{
	int	i;
	int	rc;
FN;
	for (i = 1; i < argc; i++) {
		rc = Mknode(argv[i], S_IFDIR);
		if (rc) {
			printf("Couldn't mkdir %s\n", argv[i]);
			return rc;
		}
	}
	return 0;
}

int inusep (int argc, char *argv[])
{
	//baudit("cmd line");
	binuse();
	return 0;
}

static void fill (char *b, snint n)
{
	snint	i;

	for (i = 0; i < n; i++) {
		*b++ = random();
	}
}

static void fill_file (u64 key, snint numbytes)
{
	static char	Buf[BLK_SIZE*2];
	snint		chunk;
	snint		written;

	srandom(key);
	while (numbytes) {
		if (numbytes > sizeof(Buf)) {
			chunk = sizeof(Buf);
		} else {
			chunk = numbytes;
		}
		fill(Buf, chunk);
		written = Write(key, Buf, chunk);
		if (written != chunk) {
			printf("Write failed %ld\n", written);
		}
		numbytes -= chunk;
	}
}

static void check (char *b, snint n)
{
	snint	i;
	char	c;

	for (i = 0; i < n; i++) {
		c = random();
		if (*b != c) {
			printf("check failed at %ld %d!=%d\n",
				i, *b, c);
		}
		++b;
	}
}

static void check_file (u64 key, snint numbytes)
{
	static char	Buf[BLK_SIZE*2];
	snint		chunk;
	snint		nread;

	srandom(key);
	while (numbytes) {
		if (numbytes > sizeof(Buf)) {
			chunk = sizeof(Buf);
		} else {
			chunk = numbytes;
		}
		nread = Read(key, Buf, chunk);
		if (nread != chunk) {
			printf("Read failed %ld\n", nread);
		}
		check(Buf, chunk);
		numbytes -= chunk;
	}
}

int rwp (int argc, char *argv[])
{
	u64	key;
	char	*file;
	int	rc;
FN;
	if (argc == 1) {
		file = "testfile";
	} else {
		file = argv[1];
	}
	rc = Creat(file);
	if (rc) {
		printf("Creat of %s failed %d\n", file, rc);
		return rc;
	}
	rc = Open(file, &key);
	if (rc) {
		printf("Open of %s failed %d\n", file, rc);
		return rc;
	}
	fill_file(key, 1000);
	Seek(key, 0);
	check_file(key, 1000);
	rc = Close(key);
	if (rc) {
		printf("Close of %llx failed %d\n", key, rc);
		return rc;
	}
	return 0;
}

void init_cmd (void)
{
	CMD(q,     "         # quit");
	CMD(inuse, "         # dump inuse cache buffers");
	CMD(gen,   "[n]      # generate n random directories, defaults to 10");
	CMD(mkdir, "<name>*  # create the named directories");
	CMD(ls,    "<name>*  # list content of directories");
	CMD(inodes,"         # list content of directories");
	CMD(cr,    "<name>+  # create the named files");
	CMD(rw,    "<name>+  # read/write test of given files");
}

int main (int argc, char *argv[])
{
	char	*dev = ".btree";

//	debugoff();
	debugon();
FN;	
	if (argc > 1) {
		dev = argv[1];
	}
	unlink(dev);	/* Development only */
	binit(20);
	init_fs(dev);
	init_shell(NULL);
	init_cmd();
	return shell();
}
