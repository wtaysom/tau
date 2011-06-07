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
#include <mstring.h>

extern tree_species_s	String_species;

tree_s	Tree;

enum {	UNIQUE_SHIFT = 8,
	UNIQUE_MASK = (1 << UNIQUE_SHIFT) - 1,
	HASH_MASK = ~UNIQUE_MASK };

u64 hash_string (char *s)
{
	/* Make sure sign bit is cleared and
	 * preserve the most random bits
	 */
	return (hash_string_64(s) >> (UNIQUE_SHIFT + 1)) << UNIQUE_SHIFT;
}

//===================================================================

typedef struct delete_search_s {
	char	*name;
	u64	key;
} delete_search_s;

static int delete_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	delete_search_s	*s = data;
	char		*name = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, name) == 0) {
		s->key = rec_key;
		return 0;
	}
	return qERR_TRY_NEXT;
}

int delete_string (char *name)
{
	int		rc;
	delete_search_s	s;

	s.key = hash_string(name);
	s.name = name;
	rc = search( &Tree, s.key, delete_search, &s);
	if (rc) {
		printf("delete_string [%d] couldn't find %s\n", rc, name);
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
	u64	key;
	u64	set_new_key;
} insert_search_s;

static int insert_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	insert_search_s	*s = data;
	char		*name = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, name) == 0) {
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

static int pack_string (void *to, void *from, unint len)
{
	memcpy(to, from, len);
	return 0;
}

int insert_string (char *name)
{
	int		rc;
	insert_search_s	s;

	s.key = hash_string(name);
	rc = insert( &Tree, s.key, name, strlen(name)+1);
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
	u64	key;
} find_search_s;

static int find_search (
	void	*data,
	u64	rec_key,
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

int find_string (char *name)
{
	int		rc;
	find_search_s	s;

	s.key = hash_string(name);
	s.name = name;

	rc = search( &Tree, s.key, find_search, &s);
	return rc;
}

//===================================================================

typedef struct next_search_s {
	u64	key;
	char	*name;
} next_search_s;

static int next_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	next_search_s	*s = data;

	strncpy(s->name, rec, len);
	s->name[len] = '\0';
	s->key = rec_key + 1;
	return 0;
}

int next_string (
	u64	key,
	u64	*next_key,
	char	*name)
{
	next_search_s	s;
	int		rc;

	s.key = key;
	s.name = name;
	rc = search( &Tree, s.key, next_search, &s);
	if (!rc) {
		*next_key = s.key;
	}
	return rc;
}

//===================================================================


char *dump_string (tree_s *tree, u64 rec_key, void *rec, unint len)
{
	char	*s = rec;

	return s;
}

void *root_string (tree_s *tree)
{
FN;
	return tree->t_root;
}

int change_root_string (tree_s *tree, void *root)
{
FN;
	tree->t_root = root;
	return 0;
}

void init_string (void)
{
	init_tree( &Tree, &String_species);
}

//===================================================================

tree_species_s	String_species = {
	"String",
	ts_dump:	dump_string,
	ts_root:	root_string,
	ts_change_root:	change_root_string,
	ts_pack:	pack_string};

