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

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <crc.h>
#include <btree.h>
#include <b_dir.h>
#include <b_inode.h>

enum {	UNIQUE_SHIFT = 8,
	UNIQUE_MASK = (1 << UNIQUE_SHIFT) - 1,
	HASH_MASK = ~UNIQUE_MASK };

typedef struct dir_s {
	u64	d_ino;
	char	d_name[0];
} dir_s;

static u64 hash_dir (char *s)
{
	/* Make sure sign bit is cleared and
	 * preserve the least significant byte
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
	dir_s		*dir = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, dir->d_name) == 0) {
		s->key = rec_key;
		return 0;
	}
	return qERR_TRY_NEXT;
}

int delete_dir (info_s *parent, char *name)
{
	int		rc;
	delete_search_s	s;

	s.key = hash_dir(name);
	s.name = name;
	rc = search( &parent->in_tree, s.key, delete_search, &s);
	if (rc) {
		printf("delete_dir [%d] couldn't find %s\n", rc, name);
		return rc;
	}
	rc = delete( &parent->in_tree, s.key);
	if (rc) {
		printf("Failed to delete string \"%s\" err=%d\n",
			name, rc);
	}
	return 0;
}

//===================================================================

typedef struct insert_search_s {
	u64	key;
	u64	set_new_key;
	u64	ino;
	char	*name;
} insert_search_s;

static int insert_search (
	void	*data,
	u64	rec_key,
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

static int pack_dir (void *to, void *from, unint len)
{
	insert_search_s	*s = from;
	dir_s	*dir = to;

	assert(strlen(s->name) + 1 + sizeof(dir->d_ino) == len);
	dir->d_ino = s->ino;
	strcpy(dir->d_name, s->name);
	return 0;
}

int insert_dir (info_s *parent, u64 ino, char *name)
{
	insert_search_s	s;
	unint		len;
	int		rc;

	s.key = hash_dir(name);
	s.ino = ino;
	s.name = name;
	len = strlen(name) + 1 + sizeof(s.ino);
	rc = insert( &parent->in_tree, s.key, &s, len);
	if (rc == 0) return 0;

	if (rc != qERR_DUP) {
		printf("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}

	s.set_new_key = FALSE;	// XXX: What is this for? Study more.
	rc = search( &parent->in_tree, s.key, insert_search, &s);
	if (rc == qERR_DUP) {
		printf("Duplicate string found \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	rc = insert( &parent->in_tree, s.key, &s, len);
	if (rc) {
		printf("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct find_search_s {
	u64	key;
	u64	ino;
	char	*name;
} find_search_s;

static int find_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	find_search_s	*s = data;
	dir_s		*dir = rec;

	if ((s->key & HASH_MASK) < (rec_key & HASH_MASK)) {
		return qERR_NOT_FOUND;
	}
	assert((s->key & HASH_MASK) == (rec_key & HASH_MASK));
	if (strcmp(s->name, dir->d_name) == 0) {
		s->ino = dir->d_ino;
		return 0;
	}
	return qERR_TRY_NEXT;
}

u64 find_dir (info_s *parent, char *name)
{
	int		rc;
	find_search_s	s;

	s.key = hash_dir(name);
	s.ino = 0;
	s.name = name;

	rc = search( &parent->in_tree, s.key, find_search, &s);
	if (rc) return 0;
	return s.ino;
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
	dir_s		*dir = rec;
FN;
	strcpy(s->name, dir->d_name);
	s->key = rec_key + 1;
	return 0;
}

int next_dir (
	info_s	*parent,
	u64	key,
	u64	*next_key,
	char	*name)
{
	next_search_s	s;
	int		rc;
FN;
	s.key = key;
	s.name = name;
	rc = search( &parent->in_tree, s.key, next_search, &s);
	if (!rc) {
		*next_key = s.key;
	}
	return rc;
}

//===================================================================

void dump_name (tree_s *tree, u64 rec_key, void *rec, unint len)
{
	dir_s	*dir = rec;

	printf("%lld %s", dir->d_ino, dir->d_name);
}

void dump_dir (tree_s *tree, u64 root)
{
	tree_s	t;

	if (!root) {
		printf("<empty dir>");
		return;
	}
PRd(root);
	t.t_species = &Dir_species;
	t.t_dev  = tree->t_dev;
	dump_tree( &t);
}

static u64 root_dir (tree_s *tree)
{
	info_s	*info = container(tree, info_s, in_tree);

	return info->in_inode.i_root;
}

static int change_root_dir (tree_s *tree, buf_s *root)
{
	info_s	*info = container(tree, info_s, in_tree);
	int	rc;
FN;
	info->in_inode.i_root = root->b_blkno;
	rc = update_inode( &info->in_inode);
	return rc;
}

//===================================================================

tree_species_s	Dir_species = {
	"Directory",
	ts_dump:	dump_name,
	ts_root:	root_dir,
	ts_change_root:	change_root_dir,
	ts_pack:	pack_dir};

