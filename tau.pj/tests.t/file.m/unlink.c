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
 *      unlink - remove directory entry
 * 
 * SYNOPSIS
 *      #include <unistd.h>
 * 
 *      int
 *      unlink(const char *path);
 * 
 * DESCRIPTION
 *      The unlink() function removes the link named by path from its directory
 *      and decrements the link count of the file which was referenced by the
 *      link.  If that decrement reduces the link count of the file to zero, and
 *      no process has the file open, then all resources associated with the file
 *      are reclaimed.  If one or more process have the file open when the last
 *      link is removed, the link is removed, but the removal of the file is
 *      delayed until all references to it have been closed.
 * 
 * RETURN VALUES
 *      Upon successful completion, a value of 0 is returned.  Otherwise, a value
 *      of -1 is returned and errno is set to indicate the error.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <puny.h>

int main (int argc, char *argv[])
{
	int		rc;

	punyopt(argc, argv, NULL, NULL);
	if (argc != 2) {
		fprintf(stderr, "unlink file\n");
		return 1;
	}
	rc = unlink(Option.file);
	if (rc == -1) {
		rc = errno;
		fprintf(stderr, "unlink %s: %s\n",
				Option.file, strerror(errno));
		return rc;
	}
	return 0;
}
