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
#include <sys/xattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <style.h>
#include <eprintf.h>

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
	fprintf(stderr, "%s <file> <attribute> <value>\n",
		getprogname());
	exit(1);
}

int main (int argc, char *argv[])
{
	char	*file = "";
	char	*xattr = "";
	char	*val = "";
	char	value[1024];
	ssize_t	size;
	ssize_t	rc;

	setprogname(argv[0]);

	if (argc < 4) {
		usage();
	}
	if (argc < 5) {
		file  = argv[1];
		xattr = argv[2];
		val   = argv[3];
	} else {
		usage();
	}
	rc = setxattr(file, xattr, val, strlen(val), 0);
	if (rc) {
		eprintf("setxattr file=%s xattr=%s value=%s returned:",
			file, xattr,val);
	}

	size = getxattr(file, xattr, value, 0);
	if (size < 0) {
		eprintf("getxattr=%d file=%s xattr=%s returned:",
			size, file, xattr);
	}
	if (size > 0) {
		printf("Calling getxattr with %ld\n", (unint)size);
		rc = getxattr(file, xattr, value, size);
		if (rc != size) {
			printf("Expected %ld bytes but got %ld\n",
				(unint)size, (snint)rc);
		}
	}

	printf("%s=\n", xattr);
	dumpmem(value, size);

	return 0;
}
