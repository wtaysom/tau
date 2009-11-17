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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <myio.h>

void chdirq (const char *path)
{
	if (chdir(path)) {
		io_error("chdir", path);
	}
}

void closeq (int fd)
{
	if (close(fd)) {
		io_error("close", NULL);
	}
}

void fstatq (int fd, struct stat *sb)
{
	if (fstat(fd, sb)) {
		io_error("fstat", NULL);
	}
}

void lseekq (int fd, off_t offset) 
{
	if (lseek(fd, offset, 0) == -1) {
		io_error("lseek", NULL);
	}
}

void mkdirq (const char *path)
{
	if (mkdir(path, 0777)) {
		io_error("mkdir", path);
	}
}

int openq (const char *path, int flags)
{
	int	fd;

	fd = open(path, flags, 0666);
	if (fd == -1) {
		io_error("open", path);
	}
	return fd;
}

ssize_t readq (int fd, void *buf, size_t nbytes)
{
	ssize_t	rc;

	rc = read(fd, buf, nbytes);
	if (rc == -1) {
		io_error("read", NULL);
	}
	return rc;
}

void rmdirq (const char *path)
{
	if (rmdir(path)) {
		io_error("rmdir", path);
	}
}

void statq (const char *path, struct stat *sb)
{
	if (stat(path, sb)) {
		io_error("stat", path);
	}
}

void unlinkq (const char *path)
{
	if (unlink(path)) {
		io_error("unlnk", path);
	}
}

ssize_t writeq (int fd, const void *buf, size_t nbytes)
{
	ssize_t	rc;

	rc = write(fd, buf, nbytes);
	if (rc == -1) {
		io_error("write", NULL);
	}
	if (rc != nbytes) {
#ifdef __APPLE__
		fprintf(stderr, "ERROR: write: wrote only %ld bytes of %ld\n",
#elif __x86_64__
		fprintf(stderr, "ERROR: write: wrote only %ld bytes of %ld\n",
#else
		fprintf(stderr, "ERROR: write: wrote only %d bytes of %d\n",
#endif
			rc, nbytes);
		exit(1);
	}
	return rc;
}

#if 0
void rmlevel ()
{
	DIR		*dir;
	struct dirent	*entry;
	char		name[255];

	dir = opendir(".");
	if (!dir) {
		io_error("opendir", ".");
	}
	for (;;) {
		entry = readdir(dir);
		if (entry == NULL) {
			break;
		}
printf("%s\n", entry->d_name);
		if (strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		unlink(name);
		strcpy(name, entry->d_name);
		#if 0
		if (chdir(entry->d_name)) {
			unlinkq(entry->d_name);
		} else {
			rmlevel();
			chdirq("..");
			rmdirq(entry->d_name);
		}
		#endif
	}
	closedir(dir);
}

void rmtreeq (const char *path)
{
	int	current;

	current = openq(".", O_RDONLY);
	chdir(path);

	rmlevel();

	fchdir(current);
	rmdirq(path);
}

#endif
// Do we want to do theses:
// ssize_t pread(int d, void *buf, size_t nbytes, off_t offset);
