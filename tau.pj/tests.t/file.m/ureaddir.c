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

void usage (char *name)
{
	fprintf(stderr, "%s <directory> [num_files [<num_iterations]]\n",
		name);
	exit(1);
}

int main (int argc, char *argv[])
{
	struct dirent	*de;
	char		*directory = "";
	DIR		*dir;
	unsigned	i, j;
	unsigned	numfiles = 10000;
	unsigned	n = 1000;

	if (argc < 2) {
		usage(argv[0]);
	}
	if (argc > 1) {
		directory = argv[1];
	}
	if (argc > 2) {
		numfiles = atoi(argv[2]);
	}
	if (argc > 3) {
		n = atoi(argv[3]);
	}
	mkdir(directory, 0777);
	chdirq(directory);
	create_files(numfiles);
	for (i = 0;; i++) {
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
		printf(" n=%d i=%d\n", n, i);
	}
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
