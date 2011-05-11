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
 * GETXATTR(2)                    System calls                    GETXATTR(2)
 * 
 * NAME
 *        getxattr,  lgetxattr,  fgetxattr  -  retrieve an extended attribute
 *        value
 * 
 * SYNOPSIS
 *        #include <sys/types.h>
 *        #include <sys/xattr.h>
 * 
 *        ssize_t getxattr (const char *path, const char *name,
 *                             void *value, size_t size);
 *        ssize_t lgetxattr (const char *path, const char *name,
 *                             void *value, size_t size);
 *        ssize_t fgetxattr (int filedes, const char *name,
 *                             void *value, size_t size);
 * 
 * DESCRIPTION
 *        Extended attributes are name:value  pairs  associated  with  inodes
 *        (files,  directories,  symlinks,  etc).  They are extensions to the
 *        normal attributes which are associated with all inodes in the  sys?
 *        tem  (i.e.  the  stat(2)  data).   A  complete overview of extended
 *        attributes concepts can be found in attr(5).
 * 
 *        getxattr retrieves the value of the extended  attribute  identified
 *        by  name and associated with the given path in the filesystem.  The
 *        length of the attribute value is returned.
 * 
 *        lgetxattr is identical to getxattr, except in the case  of  a  sym?
 *        bolic  link,  where  the  link itself is interrogated, not the file
 *        that it refers to.
 * 
 *        fgetxattr is identical to getxattr, only the open file  pointed  to
 *        by  filedes  (as  returned  by open(2)) is interrogated in place of
 *        path.
 * 
 *        An extended attribute name is a simple NULL-terminated string.  The
 *        name  includes  a namespace prefix - there may be several, disjoint
 *        namespaces associated with an individual inode.  The  value  of  an
 *        extended  attribute  is a chunk of arbitrary textual or binary data
 *        of specified length.
 * 
 *        An empty buffer of size zero can be  passed  into  these  calls  to
 *        return  the current size of the named extended attribute, which can
 *        be used to estimate the size of  a  buffer  which  is  sufficiently
 *        large to hold the value associated with the extended attribute.
 * 
 *        The  interface  is  designed  to  allow  guessing of initial buffer
 *        sizes, and to enlarge buffers when the return value indicates  that
 *        the buffer provided was too small.
 * 
 * RETURN VALUE
 *        On  success,  a  positive number is returned indicating the size of
 *        the extended attribute value.  On failure, -1 is returned and errno
 *        is set appropriately.
 * 
 *        If the named attribute does not exist, or the process has no access
 *        to this attribute, errno is set to ENOATTR.
 * 
 *        If the size of the value buffer is too small to  hold  the  result,
 *        errno is set to ERANGE.
 * 
 *        If  extended attributes are not supported by the filesystem, or are
 *        disabled, errno is set to ENOTSUP.
 * 
 *        The errors documented for the stat(2) system call are also applica?
 *        ble here.
 * 
 * AUTHORS
 *        Andreas  Gruenbacher,  <a.gruenbacher@computer.org> and the SGI XFS
 *        development team, <linux-xfs@oss.sgi.com>.   Please  send  any  bug
 *        reports or comments to these addresses.
 * 
 * SEE ALSO
 *        getfattr(1),  setfattr(1),  open(2),  stat(2),  setxattr(2), listx?
 *        attr(2), removexattr(2), and attr(5).
 * 
 * Dec 2001                    Extended Attributes                GETXATTR(2)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/xattr.h>

#include <eprintf.h>
#include <puny.h>

/* dumpmem: dumps an n byte area of memory to screen */
void dumpmem (const void *mem, unsigned int n)
{
	enum { NLONGS = 4,
		NCHARS = NLONGS * sizeof(long) };

	const unsigned long	*p = mem;
	const char		*c = mem;
	unsigned		i, j;
	unsigned		q, r;

	q = n / NCHARS;
	r = n % NCHARS;
	for (i = 0; i < q; i++) {
		printf("%8p:", p);
		for (j = 0; j < NLONGS; j++) {
			printf(" %8lx", *p++);
		}
		printf(" | ");
		for (j = 0; j < NCHARS; j++, c++) {
			printf("%c", isprint(*c) ? *c : '.');
		}
		printf("\n");
	}
	if (!r) return;
	printf("%8p:", p);
	for (j = 0; j < r / sizeof(long); j++) {
		printf(" %8lx", *p++);
	}
	for (; j < NLONGS; j++) {
		printf("         ");
	}
	printf(" | ");
	for (j = 0; j < r; j++, c++) {
		printf("%c", isprint(*c) ? *c : '.');
	}
	printf("\n");
}


void usage (void)
{
	pr_usage("-f<file> -x<attribute>");
}

int main (int argc, char *argv[])
{
	char	*file = "";
	char	*xattr = "";
	ssize_t	size;
	char	value[1024];

	punyopt(argc, argv, NULL, NULL);
	file = Option.file;
	xattr = Option.xattr;

	size = getxattr(file, xattr, value, sizeof(value));
	if (size < 0) {
		eprintf("file=%s xattr=%s returned:", file, xattr);
	}

	printf("%s=\n", xattr);
	dumpmem(value, size);

	return 0;
}
