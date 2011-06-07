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
#include <string.h>
#include <assert.h>

#include <debug.h>
#include <eprintf.h>
#include <crc.h>
#include <mtree.h>
#include <dir.h>

extern tree_species_s	Dir_species;

tree_s	Tree;

enum {	UNIQUE_SHIFT = 8,
	UNIQUE_MASK = (1 << UNIQUE_SHIFT) - 1,
	HASH_MASK = ~UNIQUE_MASK };

u32 hash_dir (char *s)
{
	/* Make sure sign bit is cleared and
	 * preserve the most random bits
	 */
	return (hash_string_32(s) >> (UNIQUE_SHIFT + 1)) << UNIQUE_SHIFT;
}

//===================================================================

typedef struct delete_search_s {
	char	*name;
	u32	key;
} delete_search_s;

static int delete_search (
	void	*data,
	u32	rec_key,
	void	*rec,
	unint	len)
{
	delete_search_s	*s = data;
	char		*r = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, &r[5]) == 0) {
		s->key = rec_key;
		return 0;
	}
	return qERR_TRY_NEXT;
}

int delete_dir (char *name)
{
	int		rc;
	delete_search_s	s;

	s.key = hash_dir(name);
	s.name = name;
	rc = search( &Tree, s.key, delete_search, &s);
	if (rc) {
		printf("delete_dir [%d] couldn't find %s\n", rc, name);
		return rc;
	}
	rc = delete( &Tree, s.key);
	if (rc) {
		printf("Failed to delete string \"%s\" err=%d\n",
			name, rc);
	}
	return 0;
}

//===================================================================

typedef struct insert_search_s {
	char	*name;
	u32	key;
	u32	set_new_key;
	u32	ino;
	u8	type;
} insert_search_s;

static int insert_search (
	void	*data,
	u32	rec_key,
	void	*rec,
	unint	len)
{
	insert_search_s	*s = data;
	dir_s		*dir = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, dir->d_name) == 0) {
		return qERR_DUP;
	}
	if (!s->set_new_key) {
		if (s->key != rec_key) {
			s->set_new_key = TRUE;
		} else {
			s->key = rec_key + 1;
			if ((s->key & UNIQUE_MASK) == 0) {
printf("OVERFLOW: %s\n", s->name);
				return qERR_HASH_OVERFLOW;
			}
		}
	}
	return qERR_TRY_NEXT;
}

static int pack_dir (void *to, void *from, unint size)
{
	insert_search_s	*s = from;
	u8		*t = to;

	memcpy(t, &s->ino, sizeof(s->ino));
	t += sizeof(s->ino);
	*t++ = s->type;
	size -= sizeof(s->ino) + 1;
	memcpy(t, s->name, size);
	return 0;
}

int insert_dir (char *name, u32 ino, u8 type)
{
	int		rc;
	insert_search_s	s;
	unint		size;

	size = strlen(name) + 1 + sizeof(type) + sizeof(ino);

	s.key  = hash_dir(name);
	s.name = name;
	s.ino  = ino;
	s.type = type;

	rc = insert( &Tree, s.key, &s, size);
	if (rc == 0) return 0;

	if (rc != qERR_DUP) {
		printf("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}

	s.name = name;
	s.set_new_key = FALSE;

	rc = search( &Tree, s.key, insert_search, &s);
	if (rc == qERR_DUP) {
		printf("Duplicate string found \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	rc = insert( &Tree, s.key, name, strlen(name)+1);
	if (rc) {
		printf("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct find_search_s {
	char	*name;
	u32	key;
} find_search_s;

static int find_search (
	void	*data,
	u32	rec_key,
	void	*rec,
	unint	len)
{
	find_search_s	*s = data;
	char		*name = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, name) == 0) {
		return 0;
	}
	return qERR_TRY_NEXT;
}

int find_dir (char *name)
{
	int		rc;
	find_search_s	s;

	s.key = hash_dir(name);
	s.name = name;

	rc = search( &Tree, s.key, find_search, &s);
	return rc;
}

//===================================================================

typedef struct next_search_s {
	u32	key;
	dir_s	*dir;
} next_search_s;

static int next_search (
	void	*data,
	u32	rec_key,
	void	*rec,
	unint	len)
{
	next_search_s	*s = data;
	dir_s		*r = rec;
	dir_s		*t = s->dir;

	strcpy(t->d_name, r->d_name);
	t->d_ino  = r->d_ino;
	t->d_type = r->d_type;
	s->key    = rec_key + 1;
	return 0;
}

int next_dir (
	u32	key,
	u32	*next_key,
	dir_s	*dir)
{
	next_search_s	s;
	int		rc;

	s.key = key;
	s.dir = dir;
	rc = search( &Tree, s.key, next_search, &s);
	if (!rc) {
		*next_key = s.key;
	}
	return rc;
}

//===================================================================


char *dump_dir (tree_s *tree, u32 rec_key, void *rec, unint len)
{
	static char	dump[512];
	dir_s	*d = rec;

	sprintf(dump, "%8x %x %s", d->d_ino, d->d_type, d->d_name);

	return dump;
}

void *root_dir (tree_s *tree)
{
FN;
	return tree->t_root;
}

int change_root_dir (tree_s *tree, void *root)
{
FN;
	tree->t_root = root;
	return 0;
}

void init_dir (void)
{
	init_tree( &Tree, &Dir_species);
}

//===================================================================

tree_species_s	Dir_species = {
	"Dir",
	ts_dump:	dump_dir,
	ts_root:	root_dir,
	ts_change_root:	change_root_dir,
	ts_pack:	pack_dir};

