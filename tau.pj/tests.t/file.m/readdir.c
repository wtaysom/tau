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

/*
 * READDIR(3)              Linux Programmer's Manual             READDIR(3)
 *
 *
 *
 * NAME
 *        readdir - read a directory
 *
 * SYNOPSIS
 *        #include <sys/types.h>
 *
 *        #include <dirent.h>
 *
 *        struct dirent *readdir(DIR *dir);
 *
 * DESCRIPTION
 *        The  readdir()  function  returns a pointer to a dirent structure
 *        representing the next directory entry  in  the  directory  stream
 *        pointed  to  by dir.  It returns NULL on reaching the end-of-file
 *        or if an error occurred.
 *
 *        On Linux, the dirent structure is defined as follows:
 *
 *           struct dirent {
 *               ino_t          d_ino;       // inode number
 *               off_t          d_off;       // offset to the next dirent
 *               unsigned short d_reclen;    // length of this record
 *               unsigned char  d_type;      // type of file
 *               char           d_name[256]; // filename
 *           };
 *
 *        According to POSIX, the dirent structure contains  a  field  char
 *        d_name[]  of  unspecified  size, with at most NAME_MAX characters
 *        preceding the terminating null byte.  POSIX.1-2001 also documents
 *        the  field  ino_t d_ino as an XSI extension.  Use of other fields
 *        will harm the portability of your programs.
 *
 *        The data returned by readdir() may be overwritten  by  subsequent
 *        calls to readdir() for the same directory stream.
 *
 * RETURN VALUE
 *        The  readdir()  function returns a pointer to a dirent structure,
 *        or NULL if an error occurs or end-of-file is reached.  On  error,
 *        errno is set appropriately.
 *
 * ERRORS
 *        EBADF  Invalid directory stream descriptor dir.
 *
 * CONFORMING TO
 *        SVr4, 4.3BSD, POSIX.1-2001
 *
 * SEE ALSO
 *        read(2), closedir(3), dirfd(3), ftw(3), opendir(3), rewinddir(3),
 *        scandir(3), seekdir(3), telldir(3)
 */
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <puny.h>

void pr_dirent (struct dirent *d)
{
#if 0
	printf("%10llx\n", d->d_off);
	printf("%10llx %10llx %2x %s\n",
		d->d_ino, d->d_off, d->d_type, d->d_name);
#endif
	printf("%10lld\n", (s64)d->d_off);
}

void do_readdir (char *name)
{
	DIR		*dir;
	struct dirent	*d;

	dir = opendir(name);
	if (!dir) {
		eprintf("failed to open file %s :", name);
	}
	for (;;) {
		d = readdir(dir);
		if (!d) break;
		pr_dirent(d);
	}
}

void usage (void)
{
	pr_usage("[<directory>]*");
}

int main (int argc, char *argv[])
{
	int	i;

	punyopt(argc, argv, NULL, NULL);
	if (argc == optind) {
		do_readdir(Option.dir);
	} else for (i = optind; i < argc; ++i) {
		do_readdir(argv[i]);
	}
	return 0;
}
