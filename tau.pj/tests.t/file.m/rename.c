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
 * NAME
 *      rename - change the name of a file
 *
 * SYNOPSIS
 *      #include <stdio.h>
 *
 *      int
 *      rename(const char *from, const char *to);
 *
 * DESCRIPTION
 *      Rename() causes the link named from to be renamed as to.  If to exists,
 *      it is first removed.  Both from and to must be of the same type (that is,
 *      both directories or both non-directories), and must reside on the same
 *      file system.
 *
 *      Rename() guarantees that an instance of to will always exist, even if the
 *      system should crash in the middle of the operation.
 *
 *      If the final component of from is a symbolic link, the symbolic link is
 *      renamed, not the file or directory to which it points.
 *
 * CAVEAT
 *      The system can deadlock if a loop in the file system graph is present.
 *      This loop takes the form of an entry in directory `a', say `a/foo', being
 *      a hard link to directory `b', and an entry in directory `b', say `b/bar',
 *      being a hard link to directory `a'.  When such a loop exists and two sep-
 *      arate processes attempt to perform `rename a/foo b/bar' and `rename b/bar
 *      a/foo', respectively, the system may deadlock attempting to lock both
 *      directories for modification.  Hard links to directories should be
 *      replaced by symbolic links by the system administrator.
 *
 * RETURN VALUES
 *      A 0 value is returned if the operation succeeds, otherwise rename()
 *      returns -1 and the global variable errno indicates the reason for the
 *      failure.
 *
 * ERRORS
 *      Rename() will fail and neither of the argument files will be affected if:
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <puny.h>
#include <eprintf.h>

int main (int argc, char *argv[])
{
	int	rc;

	punyopt(argc, argv, NULL, NULL);
	rc = rename(Option.file, Option.dest);
	if (rc == -1) {
		fatal("rename %s to %s:", Option.file, Option.dir);
	}
	return 0;
}
