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

/*
 * CHDIR(2)                      Linux Programmer's Manual                      CHDIR(2)
 *
 *
 *
 * NAME
 *       chdir, fchdir - change working directory
 *
 * SYNOPSIS
 *       #include <unistd.h>
 *
 *       int chdir(const char *path);
 *       int fchdir(int fd);
 *
 * DESCRIPTION
 *       chdir changes the current directory to that specified in path.
 *
 *       fchdir is identical to chdir, only that the directory is given as an open file
 *       descriptor.
 *
 * RETURN VALUE
 *       On success, zero is returned.  On error, -1 is  returned,  and  errno  is  set
 *       appropriately.
 *
 * ERRORS
 *       Depending  on the file system, other errors can be returned.  The more general
 *       errors for chdir are listed below:
 *
 *       EFAULT path points outside your accessible address space.
 *
 *       ENAMETOOLONG
 *              path is too long.
 *
 *       ENOENT The file does not exist.
 *
 *       ENOMEM Insufficient kernel memory was available.
 *
 *       ENOTDIR
 *              A component of path is not a directory.
 *
 *       EACCES Search permission is denied on a component of path.
 *
 *       ELOOP  Too many symbolic links were encountered in resolving path.
 *
 *       EIO    An I/O error occurred.
 *
 *       The general errors for fchdir are listed below:
 *
 *       EBADF  fd is not a valid file descriptor.
 *
 *       EACCES Search permission was denied on the directory open on fd.
 *
 * NOTES
 *       The prototype for fchdir is only available if _BSD_SOURCE is  defined  (either
 *       explicitly, or implicitly, by not defining _POSIX_SOURCE or compiling with the
 *       -ansi flag).
 *
 * CONFORMING TO
 *       The chdir call is compatible with SVr4, SVID,  POSIX,  X/OPEN,  4.4BSD.   SVr4
 *       documents additional EINTR, ENOLINK, and EMULTIHOP error conditions but has no
 *       ENOMEM.  POSIX.1 does not have ENOMEM or ELOOP error conditions.  X/OPEN  does
 *       not have EFAULT, ENOMEM or EIO error conditions.
 *
 *       The  fchdir  call  is compatible with SVr4, 4.4BSD and X/OPEN.  SVr4 documents
 *       additional EIO, EINTR, and ENOLINK error conditions.  X/OPEN  documents  addi?
 *       tional EINTR and EIO error conditions.
 *
 * SEE ALSO
 *       getcwd(3), chroot(2)
 *
 *
 *
 * Linux 2.0.30                          1997-08-21                             CHDIR(2)
 */


int main (int argc, char *argv[])
{
	int		rc;

	if (argc != 2) {
		fprintf(stderr, "chdir file\n");
		return 1;
	}
	rc = chdir(argv[1]);
	if (rc == -1) {
		rc = errno;
		fprintf(stderr, "chdir %s: %s\n",
				argv[1], strerror(errno));
		return rc;
	}
	return 0;
}
