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
#include <string.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <crc.h>

enum {	NAME_HASH = 1543,
	NAME_SZ   = 255,
	SLASH     = '/',
	EOS       = '\0'};

typedef struct name_s {
	struct name_s	*nm_next;
	u64		nm_parent;
	int		nm_fd;
	char		nm_name[0];
} name_s;

static name_s	*Bucket[NAME_HASH];

static name_s **hash (u64 parent, char *name)
{
	return &Bucket[(hash_string_32(name) + parent) % NAME_HASH];
}

int add_name (u64 parent, int fd, char *name)
{
	name_s	*nm;
	name_s	**b;

	nm = ezalloc(sizeof(*nm) + strlen(name) + 1);
	nm->nm_parent = parent;
	nm->nm_fd     = fd;
	strcpy(nm->nm_name, name);

	b = hash(parent, name);
	nm->nm_next = *b;
	*b = nm;

	return 0;
}

name_s *lookup_name (u64 parent, char *name)
{
	name_s	*next;
	name_s	*prev;
	name_s	**b;

	b = hash(parent, name);
	next = *b;
	if (!next) return NULL;

	if ((next->nm_parent == parent) && (strcmp(next->nm_name, name) == 0)) {
		return next;
	}
	for (;;) {
		prev = next;
		next = prev->nm_next;
		if (!next) return NULL;

		if ((next->nm_parent == parent) && (strcmp(next->nm_name, name) == 0)) {
			prev->nm_next = next->nm_next;
			next->nm_next = *b;
			*b = next;
			return next;
		}
	}
}

void delete_name (u64 parent, char *name)
{
	name_s	*next;
	name_s	*prev;
	name_s	**b;

	b = hash(parent, name);
	next = *b;
	if (!next) return;

	if ((next->nm_parent == parent) && (strcmp(next->nm_name, name) == 0)) {
		*b = next->nm_next;
		return;
	}
	for (;;) {
		prev = next;
		next = prev->nm_next;
		if (!next) return;

		if ((next->nm_parent == parent) && (strcmp(next->nm_name, name) == 0)) {
			prev->nm_next = next->nm_next;
			return;
		}
	}
}

const char *next_name (const char *path, char *name)
{
	unint	length;
FN;
	if (path == NULL) {
		return NULL;
	}
	/* Skip leading slashes */
	while (*path == SLASH) {
		++path;
	}
	for (length = 0;;) {
		if (*path == EOS){
			if (!length) path = NULL;
			break;
		}
		if (*path == SLASH) {
			++path;
			break;
		}
		if (length < NAME_SZ) {
			*name++ = *path++;
			++length;
		}
	}
	*name = EOS;
	return path;
}

#if 0
find_name (const char *path)
{
	char	name[NAME_SZ+1];
	u64	parent;
	u64	child;
FN;
	for (parent = ROOT;; parent = child) {
		path = next_name(path, name);
PRs(path);
		if (!path) break;
PRs(name);
		child = dir_lookup(parent, name);
		if (!child) {
			child = dir_create();
			dir_add(parent, child, name);
		}
	}
}
#endif
