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
 * LISTXATTR(2)             Linux Programmer's Manual            LISTXATTR(2)
 * 
 * 
 * 
 * NAME
 *        listxattr, llistxattr, flistxattr - list extended attribute names
 * 
 * SYNOPSIS
 *        #include <sys/types.h>
 *        #include <attr/xattr.h>
 * 
 *        ssize_t listxattr (const char *path,
 *                             char *list, size_t size);
 *        ssize_t llistxattr (const char *path,
 *                             char *list, size_t size);
 *        ssize_t flistxattr (int filedes,
 *                             char *list, size_t size);
 * 
 * DESCRIPTION
 *        Extended  attributes  are  name:value  pairs associated with inodes
 *        (files, directories, symlinks, etc).  They are  extensions  to  the
 *        normal  attributes which are associated with all inodes in the sys-
 *        tem (i.e. the stat(2)  data).   A  complete  overview  of  extended
 *        attributes concepts can be found in attr(5).
 * 
 *        listxattr retrieves the list of extended attribute names associated
 *        with the given path in the filesystem.  The  list  is  the  set  of
 *        (NULL-terminated)  names,  one  after the other.  Names of extended
 *        attributes to which the calling process does not have access may be
 *        omitted  from  the  list.  The length of the attribute name list is
 *        returned.
 * 
 *        llistxattr is identical to listxattr, except in the case of a  sym-
 *        bolic  link, where the list of names of extended attributes associ-
 *        ated with the link itself is retrieved, not the file that it refers
 *        to.
 * 
 *        flistxattr is identical to listxattr, only the open file pointed to
 *        by filedes (as returned by open(2)) is  interrogated  in  place  of
 *        path.
 * 
 *        A  single  extended  attribute  name  is  a  simple NULL-terminated
 *        string.  The name includes a namespace prefix; there  may  be  sev-
 *        eral, disjoint namespaces associated with an individual inode.
 * 
 *        An  empty  buffer  of  size  zero can be passed into these calls to
 *        return the current size of the list of  extended  attribute  names,
 *        which  can be used to estimate the size of a buffer which is suffi-
 *        ciently large to hold the list of names.
 * 
 * EXAMPLES
 *        The list of names is returned as an unordered array of  NULL-termi-
 *        nated  character  strings  (attribute  names  are separated by NULL
 *        characters), like this:
 *               user.name1\0system.name1\0user.name2\0
 * 
 *        Filesystems like ext2, ext3 and  XFS  which  implement  POSIX  ACLs
 *        using extended attributes, might return a list like this:
 *               system.posix_acl_access\0system.posix_acl_default\0
 * 
 * RETURN VALUE
 *        On  success,  a  positive number is returned indicating the size of
 *        the extended attribute name list.  On failure, -1 is  returned  and
 *        errno is set appropriately.
 * 
 *        If  the  size  of  the list buffer is too small to hold the result,
 *        errno is set to ERANGE.
 * 
 *        If extended attributes are not supported by the filesystem, or  are
 *        disabled, errno is set to ENOTSUP.
 * 
 *        The errors documented for the stat(2) system call are also applica-
 *        ble here.
 * 
 * AUTHORS
 *        Andreas Gruenbacher, <a.gruenbacher@computer.org> and the  SGI  XFS
 *        development  team,  <linux-xfs@oss.sgi.com>.   Please  send any bug
 *        reports or comments to these addresses.
 * 
 * SEE ALSO
 *        getfattr(1),  setfattr(1),  getxattr(2),  open(2),  removexattr(2),
 *        setxattr(2), stat(2), attr(5)
 * 
 * 
 * 
 * Dec 2001                    Extended Attributes               LISTXATTR(2)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/xattr.h>

void dump_list (char *list, ssize_t size)
{
	int	c;
	int	i;
	int	first = 1;

	for (i = 0; i < size; i++) {
		c = list[i];

		if (c) {
			if (first) {
				putchar('\t');
				first = 0;
			}
			putchar(c);
		} else {
			putchar('\n');
			first = 1;
		}
	}
}

char *ProgName;

void usage (void)
{
	fprintf(stderr, "%s file\n", ProgName);
	exit(1);
}

char	List[1<<17];

int main (int argc, char *argv[])
{
	char	*file = "";
	ssize_t	size;

	ProgName = argv[0];

	if (argc < 2) {
		usage();
	}
	if (argc < 3) {
		file = argv[1];
	} else {
		usage();
	}

	size = listxattr(file,List, sizeof(List));
	if (size == -1) {
		perror("listxattr");
		exit(2);
	}

	printf("xattrs for %s:\n", file);
	dump_list(List, size);

	return 0;
}
