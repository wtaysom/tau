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
 * TRUNCATE(2)               Linux Programmer's Manual              TRUNCATE(2)
 *
 *
 *
 * NAME
 *        truncate, ftruncate - truncate a file to a specified length
 *
 * SYNOPSIS
 *        #include <unistd.h>
 *        #include <sys/types.h>
 *
 *        int truncate(const char *path, off_t length);
 *        int ftruncate(int fd, off_t length);
 *
 * DESCRIPTION
 *        The  truncate and ftruncate functions cause the regular file named by
 *        path or referenced by fd to be  truncated  to  a  size  of  precisely
 *        length bytes.
 *
 *        If  the  file previously was larger than this size, the extra data is
 *        lost.  If the file previously was shorter, it is  extended,  and  the
 *        extended part reads as zero bytes.
 *
 *        The file pointer is not changed.
 *
 *        If the size changed, then the ctime and mtime fields for the file are
 *        updated, and suid and sgid mode bits may be cleared.
 *
 *        With ftruncate, the file must be open for writing; with truncate, the
 *        file must be writable.
 *
 * RETURN VALUE
 *        On success, zero is returned.  On error, -1 is returned, and errno is
 *        set appropriately.
 *
 * ERRORS
 *        For truncate:
 *
 *        EACCES Search permission is denied for a component of the  path  pre?
 *               fix, or the named file is not writable by the user.
 *
 *        EFAULT Path points outside the process's allocated address space.
 *
 *        EFBIG  The  argument  length  is  larger  than the maximum file size.
 *               (XSI)
 *
 *        EINTR  A signal was caught during execution.
 *
 *        EINVAL The argument length is negative or  larger  than  the  maximum
 *               file size.
 *
 *        EIO    An I/O error occurred updating the inode.
 *
 *        EISDIR The named file is a directory.
 *
 *        ELOOP  Too  many  symbolic  links were encountered in translating the
 *               pathname.
 *
 *        ENAMETOOLONG
 *               A component of a  pathname  exceeded  255  characters,  or  an
 *               entire path name exceeded 1023 characters.
 *
 *        ENOENT The named file does not exist.
 *
 *        ENOTDIR
 *               A component of the path prefix is not a directory.
 *
 *        EROFS  The named file resides on a read-only file system.
 *
 *        ETXTBSY
 *               The  file is a pure procedure (shared text) file that is being
 *               executed.
 *
 *        For ftruncate the same errors apply, but instead of things  that  can
 *        be wrong with path, we now have things that can be wrong with fd:
 *
 *        EBADF  The fd is not a valid descriptor.
 *
 *        EBADF or EINVAL
 *               The fd is not open for writing.
 *
 *        EINVAL The fd does not reference a regular file.
 *
 * CONFORMING TO
 *        4.4BSD, SVr4 (these function calls first appeared in BSD 4.2).  POSIX
 *        1003.1-1996 has ftruncate.  POSIX 1003.1-2001 also has  truncate,  as
 *        an XSI extension.
 *
 *        SVr4 documents additional truncate error conditions EMFILE, EMULTIHP,
 *        ENFILE, ENOLINK.  SVr4 documents for ftruncate an  additional  EAGAIN
 *        error condition.
 *
 * NOTES
 *        The above description is for XSI-compliant systems.  For non-XSI-com?
 *        pliant systems, the POSIX standard allows two behaviours  for  ftrun?
 *        cate  when  length exceeds the file length (note that truncate is not
 *        specified at all in such an environment): either returning an  error,
 *        or extending the file.  (Most Unices follow the XSI requirement.)
 *
 * SEE ALSO
 *        open(2)
 *
 *
 *
 *                                  1998-12-21                      TRUNCATE(2)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <puny.h>

#define BUF_SIZE	(1<<16)

int	Buf[BUF_SIZE];

#define NUM_BUFS	100	// (((1<<30) + (1<<29)) / sizeof(Buf))

int main (int argc, char *argv[])
{
	int	fd;
	int	rc;
	char	*name;
	ssize_t	bytes;
	long	i, j;

	punyopt(argc, argv, NULL, NULL);
	name = Option.file;

	for (i = 0; i < BUF_SIZE; ++i) {
		Buf[i] = random();
	}
	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	for (j = 0; j < Option.iterations; ++j) {
		for (i = 0; i < NUM_BUFS; ++i) {
			bytes = write(fd, Buf, sizeof(Buf));
			if (bytes == -1) {
				perror("write");
				fprintf(stderr, "buffers written %ld\n", i);
				exit(1);
			}
		}
		rc = truncate(name, 0);
		if (rc == -1) {
			perror("truncate");
			exit(1);
		}
		lseek(fd, NUM_BUFS * sizeof(Buf), 0);
		if (write(fd, Buf, sizeof(Buf)) != sizeof(Buf)) {
			perror("write");
		}
		lseek(fd, 0, 0);
		for (i = 0; i < NUM_BUFS; ++i) {
			bytes = read(fd, Buf, sizeof(Buf));
			if (bytes == -1) {
				perror("read");
				fprintf(stderr, "buffers read %ld\n", i);
				exit(1);
			}
		}
	}
	close(fd);
	return 0;
}
