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
 *SYMLINK(2)                Linux Programmer's Manual                SYMLINK(2)
 *
 *
 *
 *NAME
 *       symlink - make a new name for a file
 *
 *SYNOPSIS
 *       #include <unistd.h>
 *
 *       int symlink(const char *oldpath, const char *newpath);
 *
 *DESCRIPTION
 *       symlink  creates  a  symbolic  link  named  newpath which contains the
 *       string oldpath.
 *
 *       Symbolic links are interpreted at run-time as if the contents  of  the
 *       link  had been substituted into the path being followed to find a file
 *       or directory.
 *
 *       Symbolic links may contain ..  path components, which (if used at  the
 *       start  of  the  link) refer to the parent directories of that in which
 *       the link resides.
 *
 *       A symbolic link (also known as a soft link) may point to  an  existing
 *       file  or  to a nonexistent one; the latter case is known as a dangling
 *       link.
 *
 *       The permissions of a symbolic link are irrelevant;  the  ownership  is
 *       ignored when following the link, but is checked when removal or renam?
 *       ing of the link is requested and the link is in a directory  with  the
 *       sticky bit (S_ISVTX) set.
 *
 *       If newpath exists it will not be overwritten.
 *
 *RETURN VALUE
 *       On  success, zero is returned.  On error, -1 is returned, and errno is
 *       set appropriately.
 *
 *ERRORS
 *       EACCES Write access to the directory containing newpath is denied,  or
 *              one  of  the  directories in the path prefix of newpath did not
 *              allow search permission.  (See also path_resolution(2).)
 *
 *       EEXIST newpath already exists.
 *
 *       EFAULT oldpath or  newpath  points  outside  your  accessible  address
 *              space.
 *
 *       EIO    An I/O error occurred.
 *
 *       ELOOP  Too  many symbolic links were encountered in resolving newpath.
 *
 *       ENAMETOOLONG
 *              oldpath or newpath was too long.
 *
 *       ENOENT A directory component in newpath does not exist or  is  a  dan?
 *              gling symbolic link, or oldpath is the empty string.
 *
 *       ENOMEM Insufficient kernel memory was available.
 *
 *       ENOSPC The  device  containing the file has no room for the new direc?
 *              tory entry.
 *
 *       ENOTDIR
 *              A component used as a directory in newpath is not, in  fact,  a
 *              directory.
 *
 *       EPERM  The filesystem containing newpath does not support the creation
 *              of symbolic links.
 *
 *       EROFS  newpath is on a read-only filesystem.
 *
 *NOTES
 *       No checking of oldpath is done.
 *
 *       Deleting the name referred to by a symlink will  actually  delete  the
 *       file  (unless  it also has other hard links). If this behaviour is not
 *       desired, use link.
 *
 *CONFORMING TO
 *       SVr4, SVID, POSIX, 4.3BSD.   SVr4  documents  additional  error  codes
 *       SVr4,  SVID,  4.3BSD,  X/OPEN.   SVr4 documents additional error codes
 *       EDQUOT and ENOSYS.  See open(2) re multiple files with the same  name,
 *       and NFS.
 *
 *SEE ALSO
 *       ln(1),  link(2),  lstat(2),  open(2), path_resolution(2), readlink(2),
 *       rename(2), unlink(2)
 */

#include <unistd.h>

#include <eprintf.h>
#include <mylib.h>
#include <puny.h>

char *gen_name (int len)
{
	static char file_name_char[] =  "abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";
	unsigned	i;
	char		*c;

	c = ezalloc(len);

	for (i = 0; i < len - 1; i++) {
		c[i] = file_name_char[urand(sizeof(file_name_char)-1)];
	}
	c[i] = '\0';

	return c;
}

int main (int argc, char *argv[])
{
	int	rc;
	char	*oldpath;
	char	*newpath;

	punyopt(argc, argv, NULL, NULL);
	newpath = Option.dest;
	oldpath = gen_name(Option.name_size);
	rc = symlink(oldpath, newpath);
	if (rc) eprintf("symlink %s -> %s failed:", oldpath, newpath);

	return 0;
}
