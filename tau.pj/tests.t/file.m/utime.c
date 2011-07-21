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
 *UTIME(2)            Linux Programmer's Manual            UTIME(2)
 *
 *
 *
 *NAME
 *     utime, utimes - change access and/or modification times of an inode
 *
 *SYNOPSIS
 *     #include <sys/types.h>
 *     #include <utime.h>
 *
 *     int utime(const char *filename, struct utimbuf *buf);
 *
 *
 *     #include <sys/time.h>
 *
 *     int utimes(char *filename, struct timeval *tvp);
 *
 *DESCRIPTION
 *     utime  changes  the access and modification times of the inode specified
 *     by filename to the actime and modtime fields of  buf  respectively.   If
 *     buf  is NULL, then the access and modification times of the file are set
 *     to the current time.  The utimbuf structure is:
 *
 *            struct utimbuf {
 *                    time_t actime;  // access time
 *                    time_t modtime; // modification time
 *            };
 *
 *     In the Linux DLL 4.4.1 libraries, utimes is just a  wrapper  for  utime:
 *     tvp[0].tv_sec  is  actime,  and  tvp[1].tv_sec  is modtime.  The timeval
 *     structure is:
 *
 *            struct timeval {
 *                    long    tv_sec;         // seconds
 *                    long    tv_usec;        // microsecond
 *            };
 *
 *RETURN VALUE
 *     On success, zero is returned.  On error, -1 is returned,  and  errno  is
 *     set appropriately.
 *
 *ERRORS
 *     Other errors may occur.
 *
 *
 *     EACCES Permission to write the file is denied.
 *
 *     ENOENT filename does not exist.
 *
 *CONFORMING TO
 *     utime:  SVr4,  SVID,  POSIX.  SVr4 documents additional error conditions
 *     EFAULT, EINTR, ELOOP, EMULTIHOP, ENAMETOOLONG,  ENOLINK,  ENOTDIR,  ENO
 *     LINK, ENOTDIR, EPERM, EROFS.
 *     utimes: BSD 4.3
 *
 *SEE ALSO
 *     stat(2)
 *
 *
 *
 *Linux                       1995-06-10                   UTIME(2)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <utime.h>

#include <style.h>
#include <mystdlib.h>
#include <puny.h>

enum {
	SECS_PER_MIN  = 60,
	SECS_PER_HR   = SECS_PER_MIN * 60,
	SECS_PER_DAY  = SECS_PER_HR * 24,
	SECS_PER_YEAR = SECS_PER_DAY * 365,
	SECS_PER_LEAP = SECS_PER_YEAR + SECS_PER_DAY,
	SECS_PER_4YR  = (3 * SECS_PER_YEAR) + SECS_PER_LEAP
};

char FileTypes[] = { '0', 'p', 'c', '3', 'd', '5', 'b', '7',
					 '-', '9', 'l', 'B', 's', 'D', 'E', 'F' };

void prStatNice (struct stat *sb)
{
	char	*time = date(sb->st_mtime);

	printf("%c%.3o %9llu %s ", //""%c%.3o %9qd %s",
				FileTypes[(sb->st_mode >> 12) & 017],
				sb->st_mode & 0777,	/* inode protection mode */
				(u64)sb->st_size,	/* file size, in bytes */
				time			/* time of last data modification */
			);
	printf(" %s", date(sb->st_atime));
}

int main (int argc, char *argv[])
{
	struct stat	sb;
	struct utimbuf	utbuf;
	char	*name;
	int	rc;

	punyopt(argc, argv, NULL, NULL);
	name = Option.file;
	rc = stat(name, &sb);
	if (rc == -1) {
		perror(name);
		exit(1);
	}
	prStatNice( &sb); printf(" %s\n", name);
	utbuf.actime = sb.st_atime + 100;
	//utbuf.modtime = sb.st_mtime + 200;
	utbuf.modtime = sb.st_mtime;
	rc = utime(name, &utbuf);
	if (rc == -1)
	{
		perror("utime");
		exit(2);
	}
	rc = stat(name, &sb);
	if (rc == -1) {
		perror(name);
		exit(1);
	}
	prStatNice( &sb); printf(" %s\n", name);
	return 0;
}
