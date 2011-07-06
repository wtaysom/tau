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
 * READDIR(3)               Linux Programmer's Manual              READDIR(3)
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
 *        The readdir() function returns a pointer to a dirent structure rep?
 *        resenting the next directory entry in the directory stream  pointed
 *        to  by  dir.   It returns NULL on reaching the end-of-file or if an
 *        error occurred.
 *
 *        According to POSIX, the dirent  structure  contains  a  field  char
 *        d_name[] of unspecified size, with at most NAME_MAX characters pre?
 *        ceding the terminating null character.  Use of  other  fields  will
 *        harm the portability of your programs.  POSIX 1003.1-2001 also doc?
 *        uments the field ino_t d_ino as an XSI extension.
 *
 *        The data returned by readdir() may  be  overwritten  by  subsequent
 *        calls to readdir() for the same directory stream.
 *
 * RETURN VALUE
 *        The  readdir() function returns a pointer to a dirent structure, or
 *        NULL if an error occurs or end-of-file is reached.
 *
 * ERRORS
 *        EBADF  Invalid directory stream descriptor dir.
 *
 * CONFORMING TO
 *        SVID 3, BSD 4.3, POSIX 1003.1-2001
 *
 * SEE ALSO
 *        read(2), closedir(3),  dirfd(3),  opendir(3),  rewinddir(3),  scan?
 *        dir(3), seekdir(3), telldir(3)
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <style.h>
#include <mylib.h>
#include <myio.h>
#include <eprintf.h>
#include <puny.h>

u64 Numfiles = 10000;

void create_files (unsigned n)
{
	unsigned	i;
	char		name[NAME_MAX+1];
	int		fd;

	for (i = 0; i < n; ++i) {
		sprintf(name, "f%x", i);
		fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
		if (fd == -1) {
			perror(name);
			exit(1);
		}
		close(fd);
	}
}

bool myopt (int c)
{
	switch (c) {
	case 'k':
		Numfiles = strtoll(optarg, NULL, 0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void usage (void)
{
	pr_usage("-d<directory> -k<num_files> -i<num_iterations> -l<loops>");
}

int main (int argc, char *argv[])
{
	struct dirent	*de;
	DIR		*dir;
	unsigned	i, j;
	unsigned	n;
	u64		l;

	punyopt(argc, argv, myopt, "k:");
	n = Option.iterations;
	mkdir(Option.dir, 0777);
	chdirq(Option.dir);
	create_files(Numfiles);
	for (l = 0; l < Option.loops; l++) {
		startTimer();
		for (j = 0; j < n; ++j) {
			dir = opendir(".");
			if (!dir) {
				perror(".");
				exit(1);
			}
			for (i = 0;; ++i) {
				de = readdir(dir);
				if (!de) break;
			}
			closedir(dir);
		}
		stopTimer();
		prTimer();
		printf(" n=%d l=%lld\n", n, l);
	}
	return 0;
}

/*
printf("d_ino=%d d_off=%d d_reclen=%d d_type=%d d_name=%d off_t=%d\n",
	sizeof(de->d_ino), sizeof(de->d_off), sizeof(de->d_reclen),
	sizeof(de->d_type), sizeof(de->d_name), sizeof(off_t));

printf("d_ino=%llx d_off=%llx d_reclen=%d d_type=%x d_name=%s\n",
	de->d_ino, de->d_off, de->d_reclen,
	de->d_type, de->d_name);
printf("telldir=%lx\n", telldir(dir));
*/
