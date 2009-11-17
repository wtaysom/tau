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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/xattr.h>

#include <eprintf.h>

char	*FileName;

/* dump_mem: dumps an n byte area of memory to screen */
void dump_mem (const void *mem, unsigned int n)
{
	enum { NINTS = 4,
		NCHARS = NINTS * sizeof(int) };

	const unsigned long	*p = mem;
	const char		*c = mem;
	unsigned		i, j;
	unsigned		q, r;

	q = n / NCHARS;
	r = n % NCHARS;
	for (i = 0; i < q; i++) {
		printf("\t\t%8p:", p);
		for (j = 0; j < NINTS; j++) {
			printf(" %8lx", *p++);
		}
		printf(" | ");
		for (j = 0; j < NCHARS; j++, c++) {
			printf("%c", isprint(*c) ? *c : '.');
		}
		printf("\n");
	}
	if (!r) return;
	printf("\t\t%8p:", p);
	for (j = 0; j < r / sizeof(int); j++) {
		printf(" %8lx", *p++);
	}
	for (; j < NINTS; j++) {
		printf("         ");
	}
	printf(" | ");
	for (j = 0; j < r; j++, c++) {
		printf("%c", isprint(*c) ? *c : '.');
	}
	printf("\n");
}

char	Buf[1<<17];

void dump_xattr (char *name)
{
	ssize_t		size;

	size = getxattr(FileName, name, &Buf, sizeof(Buf));
	if (size == -1) {
		eprintf("file=%s xattr=%s returned:", FileName, name);
	}
	dump_mem(Buf, size);
}

void dump_list (char *list, ssize_t size)
{
	int	c;
	int	i;
	char	*xattr = 0;
	int	first = 1;

	for (i = 0; i < size; i++) {
		c = list[i];

		if (c) {
			if (first) {
				putchar('\t');
				first = 0;
				xattr = &list[i];
			}
			putchar(c);
		} else {
			putchar('\n');
			first = 1;
			if (xattr) {
				dump_xattr(xattr);
				xattr = 0;
			}
		}
	}
}

char *ProgName;

void usage (void)
{
	fprintf(stderr, "%s {file}+\n", ProgName);
	exit(1);
}

char	List[1<<17];

int main (int argc, char *argv[])
{
	ssize_t	size;
	int	i;

	ProgName = argv[0];

	if (argc < 2) {
		usage();
	}

	for (i = 1; i < argc; i++) {
		FileName = argv[i];
		size = listxattr(FileName, List, sizeof(List));
		if (size == -1) {
			perror("listxattr");
			exit(2);
		}
		printf("xattrs for %s:\n", FileName);
		dump_list(List, size);
	}
	return 0;
}
