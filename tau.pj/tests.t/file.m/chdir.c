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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <puny.h>

/*
 * CHDIR(2)                   Linux Programmer's Manual                CHDIR(2)
 *
 * NAME
 *     chdir, fchdir - change working directory
 *
 * SYNOPSIS
 *     #include <unistd.h>
 *
 *     int chdir(const char *path);
 *     int fchdir(int fd);
 *
 * Feature Test Macro Requirements for glibc (see feature_test_macros(7)):
 *
 *     fchdir(): _BSD_SOURCE || _XOPEN_SOURCE >= 500
 *
 * DESCRIPTION
 *     chdir() changes the current working directory of the calling process to
 *     the directory specified in path.
 *
 *     fchdir() is identical to chdir();  the  only  difference  is  that  the
 *     directory is given as an open file descriptor.
 *
 * RETURN VALUE
 *     On  success,  zero is returned.  On error, -1 is returned, and errno is
 *     set appropriately.
 *
 * ERRORS
 *     Depending on the file system, other errors can be returned.   The  more
 *     general errors for chdir() are listed below:
 *
 *     EACCES Search  permission  is denied for one of the components of path.
 *            (See also path_resolution(7).)
 *
 *     EFAULT path points outside your accessible address space.
 *
 *     EIO    An I/O error occurred.
 *
 *     ELOOP  Too many symbolic links were encountered in resolving path.
 *
 *     ENAMETOOLONG
 *            path is too long.
 *
 *     ENOENT The file does not exist.
 *
 *     ENOMEM Insufficient kernel memory was available.
 *
 *     ENOTDIR
 *            A component of path is not a directory.
 *
 *     The general errors for fchdir() are listed below:
 *
 *     EACCES Search permission was denied on the directory open on fd.
 *
 *     EBADF  fd is not a valid file descriptor.
 *
 * CONFORMING TO
 *     SVr4, 4.4BSD, POSIX.1-2001.
 *
 * NOTES
 *     The current working directory is the starting  point  for  interpreting
 *     relative pathnames (those not starting with '/').
 *
 *     A child process created via fork(2) inherits its parent's current work‐
 *     ing directory.  The current working  directory  is  left  unchanged  by
 *     execve(2).
 *
 *     The prototype for fchdir() is only available if _BSD_SOURCE is defined,
 *     or _XOPEN_SOURCE is defined with the value 500.
 *
 * SEE ALSO
 *     chroot(2), getcwd(3), path_resolution(7)
 *
 * COLOPHON
 *     This page is part of release 3.23 of the Linux  man-pages  project.   A
 *     description  of  the project, and information about reporting bugs, can
 *     be found at http://www.kernel.org/doc/man-pages/.
 *
 * Linux                           2007-07-26                          CHDIR(2)
 */ 

int main (int argc, char *argv[])
{
	int		rc;

	punyopt(argc, argv, NULL, NULL);
	rc = chdir(Option.dir);
	if (rc == -1) {
		rc = errno;
		fprintf(stderr, "chdir %s: %s\n",
				argv[1], strerror(errno));
		return rc;
	}
	return 0;
}
