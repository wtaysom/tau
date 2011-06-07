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
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include <eprintf.h>
#include <debug.h>

#include <sw.h>

static name_s	*Name[MAX_NAMES];
static name_s	**NextName = Name;

static name_s *name_lookup (name_s *name)
{
	name_s	**np;

	for (np = Name; np != NextName; np++) {
		if (*np
		    && ((*np)->nm_length == name->nm_length)
		    && (memcmp((*np)->nm_name, name->nm_name, name->nm_length) == 0)) {
			return *np;
		}
	}
	return NULL;
}

name_s *name_find (name_s *name)
{
	name_s	*n;

	if (name->nm_length > TAU_NAME) {
		eprintf("Name too long %d\n", name->nm_length);
		return NULL;
	}
	n = name_lookup(name);
	if (n) return n;

	if (NextName == &Name[MAX_NAMES]) {
		eprintf("Too many names %s:", name);
	}
	n = emalloc(name->nm_length+1);
	memcpy(n, name, name->nm_length+1);
	*NextName++ = n;
	return n;
}
