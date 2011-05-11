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
 * LINK(2)             Linux Programmer's Manual             LINK(2)
 *
 *
 *
 * NAME
 *     link - make a new name for a file
 *
 * SYNOPSIS
 *     #include <unistd.h>
 *
 *     int link(const char *oldpath, const char *newpath);
 *
 * DESCRIPTION
 *     link creates a new link (also known as a hard link) to an existing file.
 *
 *     If newpath exists it will not be overwritten.
 *
 *     This new name may be used exactly as the old one for any operation; both
 *     names  refer to the same file (and so have the same permissions and own-
 *     ership) and it is impossible to tell which name was the `original'.
 *
 * RETURN VALUE
 *     On success, zero is returned.  On error, -1 is returned,  and  errno  is
 *     set appropriately.
 *
 * ERRORS
 *     EXDEV  oldpath and newpath are not on the same filesystem.
 *
 *     EPERM  The  filesystem  containing  oldpath and newpath does not support
 *            the creation of hard links.
 *
 *     EFAULT oldpath or newpath points outside your accessible address  space.
 *
 *     EACCES Write  access  to the directory containing newpath is not allowed
 *            for the process's effective uid, or one  of  the  directories  in
 *            oldpath or newpath did not allow search (execute) permission.
 *
 *     ENAMETOOLONG
 *            oldpath or newpath was too long.
 *
 *     ENOENT A  directory component in oldpath or newpath does not exist or is
 *            a dangling symbolic link.
 *
 *     ENOTDIR
 *            A component used as a directory in oldpath or newpath is not,  in
 *            fact, a directory.
 *
 *     ENOMEM Insufficient kernel memory was available.
 *
 *     EROFS  The file is on a read-only filesystem.
 *
 *     EEXIST newpath already exists.
 *
 *     EMLINK The file referred to by oldpath already has the maximum number of
 *            links to it.
 *
 *     ELOOP  Too many symbolic links were encountered in resolving oldpath  or
 *            newpath.
 *
 *     ENOSPC The  device containing the file has no room for the new directory
 *            entry.
 *
 *     EPERM  oldpath is a directory.
 *
 *     EIO    An I/O error occurred.
 *
 * NOTES
 *     Hard links, as created by link, cannot span filesystems. Use symlink  if
 *     this is required.
 *
 * CONFORMING TO
 *     SVr4,  SVID,  POSIX, BSD 4.3, X/OPEN.  SVr4 documents additional ENOLINK
 *     and EMULTIHOP error conditions; POSIX.1 does not document ELOOP.  X/OPEN
 *     does not document EFAULT, ENOMEM or EIO.
 *
 * BUGS
 *     On NFS file systems, the return code may be wrong in case the NFS server
 *     performs the link creation and dies before it can say so.   Use  stat(2)
 *     to find out if the link got created.
 *
 * SEE ALSO
 *     symlink(2), unlink(2), rename(2), open(2), stat(2), ln(1)
 *
 *
 *
 * Linux 2.0.30                1997-12-10                    LINK(2)
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <puny.h>

int main (int argc, char *argv[])
{
	int	rc;
	char	*src;
	char	*dst;

	punyopt(argc, argv, NULL, NULL);
	src = Option.file;
	dst = Option.dest;

	rc = link(src, dst);
	if (rc == -1) {
		perror(src);
		exit(1);
	}
	return 0;
}
