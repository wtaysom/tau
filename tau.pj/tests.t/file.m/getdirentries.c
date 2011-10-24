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
 * GETDIRENTRIES(3)    Linux Programmer's Manual    GETDIRENTRIES(3)
 *
 *     getdirentries - get directory entries in a filesystem independent format
 *
 * SYNOPSIS
 *     #define _BSD_SOURCE or #define _SVID_SOURCE
 *     #include <dirent.h>
 *
 *     ssize_t getdirentries(int fd, char *buf, size_t nbytes , off_t *basep);
 *
 * DESCRIPTION
 *     Read directory entries from the directory specified by fd into buf.   At
 *     most  nbytes  are  read.  Reading starts at offset *basep, and *basep is
 *     updated with the new position after reading.
 *
 * RETURN VALUE
 *     getdirentries returns the number of bytes read or zero when at  the  end
 *     of  the directory.  If an error occurs, -1 is returned, and errno is set
 *     appropriately.
 *
 * ERRORS
 *     See the Linux library source code for details.
 *
 * SEE ALSO
 *     open(2), lseek(2)
 *
 * BSD/MISC                    1993-07-22           GETDIRENTRIES(3)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <dirent.h>
#include <unistd.h>
//#include <linux/types.h>
//#include <linux/dirent.h>
//#include <linux/unistd.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <puny.h>


#ifdef __APPLE__
int main (int argc, char *argv[])
{
	fatal("getdirentires not available on APPLE with 64 bit inodes");
	return 0;
}

#else
enum { BUF_SZ = 40 };

void prDirentries (char *buf, int numBytes)
{
	struct dirent	*d;
	int	i;

	for (i = 0; i < numBytes; i += d->d_reclen) {
		d = (struct dirent *)&buf[i];
		printf("\t%10qu %s\n", (u64)d->d_ino, d->d_name);
	}
}

int doGetdirentries (char *name)	// Need to test getdirentries too.
{
	int	fd;
	int	rc;
	char	buf[BUF_SZ];
	off_t	base = 0;

	printf("%s:\n", name);

	fd = open(name, O_RDONLY);
	if (fd == -1) {
		rc = errno;
		fprintf(stderr, "open:%s %s\n", name, strerror(errno));
		return rc;
	}
	for (;;) {
		lseek(fd, base, 0);
		rc = getdirentries(fd, buf, BUF_SZ, &base);
		base = lseek(fd, 0, 1);
		if (rc == 0) break;
		if (rc == -1) {
			rc = errno;
			fprintf(stderr, "getdirentries:%s %s\n",
				name, strerror(errno));
			close(fd);
			return rc;
		}
		prDirentries(buf, rc);
	}
	close(fd);
	return 0;
}

int main (int argc, char *argv[])
{
	int		i;
	int		rc;

	punyopt(argc, argv, NULL, NULL);
	if (argc == optind) {
		rc = doGetdirentries(Option.dir);
		return rc;
	}
	for (i = optind; i < argc; ++i) {
		rc = doGetdirentries(argv[i]);
		if (rc != 0) return rc;
	}
	return 0;
}
#endif
