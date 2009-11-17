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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void *myalloc (unsigned size)
{
	void	*p;

	p = malloc(size);
	if (p == NULL) {
		fprintf(stderr,
			"myalloc couldn't allocate %d bytes of memory\n",
			size);
		exit(1);
	}
	bzero(p, size);
	return p;
}

void *myrealloc (void *p, unsigned size)
{
	void	*q;

	q = realloc(p, size);
	if (q == NULL) {
		fprintf(stderr,
			"myrealloc couldn't reallocate %d bytes of memory\n",
			size);
		exit(1);
	}
	return q;
}

static char NilString[] = "";

char *strAlloc (char *s)
{
	char	*new;

	if (s == NULL) return NilString;
	if (*s == '\0') return NilString;
	new = malloc(strlen(s) + 1);
	if (new == NULL) {
		fprintf(stderr,
			"strAlloc couldn't allocate space for %s\n",
			s);
		exit(1);
	}
	strcpy(new, s);
	return new;
}

void strFree (char *s)
{
	if (s == NilString) return;
	free(s);
}
