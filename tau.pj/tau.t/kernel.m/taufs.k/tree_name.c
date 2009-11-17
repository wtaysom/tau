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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>

#include <style.h>
#include <tau/debug.h>
#include <crc.h>
#include <tau_err.h>
#include <tau_blk.h>
#include <tau_fs.h>

enum {	UNIQUE_SHIFT = 8,
	UNIQUE_MASK = (1 << UNIQUE_SHIFT) - 1,
	HASH_MASK = ~UNIQUE_MASK };

typedef struct dir_s {
	u64	d_ino;
	char	d_name[0];
} dir_s;

//===================================================================
static char *dump_name     (tree_s *tree, u64 rec_key, void *rec, unint size);
static u64 root_dir        (tree_s *tree);
static int change_root_dir (tree_s *tree, void *root);
static int pack_dir (void *to, void *from, unint size);

tree_species_s	Tau_dir_species = {
	"Directory",
	ts_dump:	dump_name,
	ts_root:	root_dir,
	ts_change_root:	change_root_dir,
	ts_pack:	pack_dir};

//===================================================================

static u64 hash_dir (const u8 *s)
{
	/* Make sure sign bit is cleared and
	 * preserve the least significant byte
	 */
	return (hash_string_64_tau((const char *)s) >> (UNIQUE_SHIFT + 1))
			<< UNIQUE_SHIFT;
}

//===================================================================

typedef struct delete_search_s {
	const u8	*name;
	u64		key;
} delete_search_s;

static int delete_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
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

int tau_delete_dir (tree_s *ptree, const u8 *name)
{
	delete_search_s		s;
	int	rc;

	s.key = hash_dir(name);
	s.name = name;
	rc = tau_search(ptree, s.key, delete_search, &s);
	if (rc) {
		dprintk("delete_dir [%d] couldn't find %s\n", rc, name);
		return rc;
	}
	rc = tau_delete(ptree, s.key);
	if (rc) {
		eprintk("Failed to delete string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct insert_search_s {
	u64		key;
	u64		set_new_key;
	u64		ino;
	const u8	*name;
} insert_search_s;

static int insert_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
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
				eprintk("OVERFLOW: %s\n", s->name);
				return qERR_HASH_OVERFLOW;
			}
		}
	}
	return qERR_TRY_NEXT;
}

static int pack_dir (void *to, void *from, unint size)
{
	insert_search_s	*s = from;
	dir_s	*dir = to;

	assert(strlen(s->name) + 1 + sizeof(dir->d_ino) == size);
	dir->d_ino = s->ino;
	strcpy(dir->d_name, s->name);
	return 0;
}

int tau_insert_dir (tree_s *ptree, u64 ino, const u8 *name)
{
	insert_search_s	s;
	unint		size;
	int		rc;

	s.key = hash_dir(name);
	s.ino = ino;
	s.name = name;
	size = strlen(name) + 1 + sizeof(s.ino);
	rc = tau_insert(ptree, s.key, &s, size);
	if (rc == 0) return 0;

	if (rc != qERR_DUP) {
		eprintk("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}

	s.set_new_key = FALSE;	// XXX: What is this for? Study more.
	rc = tau_search(ptree, s.key, insert_search, &s);
	if (rc == qERR_DUP) {
		dprintk("Duplicate string found \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	rc = tau_insert(ptree, s.key, &s, size);
	if (rc) {
		eprintk("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct find_search_s {
	u64		key;
	u64		ino;
	const u8	*name;
} find_search_s;

static int find_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
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

u64 tau_find_dir (tree_s *ptree, const u8 *name)
{
	find_search_s	s;
	int		rc;

	s.key = hash_dir(name);
	s.ino = 0;
	s.name = name;

	rc = tau_search(ptree, s.key, find_search, &s);
	if (rc) return 0;
	return s.ino;
}

//===================================================================

typedef struct next_search_s {
	u64	key;
	u8	*name;
	u64	*ino;
} next_search_s;

static int next_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	next_search_s	*s = data;
	dir_s		*dir = rec;
FN;
	strcpy(s->name, dir->d_name);
	*s->ino = dir->d_ino;
	s->key = rec_key + 1;
	return 0;
}

int tau_next_dir (
	tree_s	*ptree,
	u64	key,
	u64	*next_key,
	u8	*name,
	u64	*ino)
{
	next_search_s	s;
	int		rc;
FN;
	s.key = key;
	s.name = name;
	s.ino  = ino;
	rc = tau_search(ptree, s.key, next_search, &s);
	if (!rc) {
		*next_key = s.key;
	}
	return rc;
}

//===================================================================

static char *dump_name (tree_s *tree, u64 rec_key, void *rec, unint size)
{
	static char	buf[2 * TAU_FILE_NAME_LEN];

	dir_s	*dir = rec;

	snprintf(buf, sizeof(buf), "%lld %s", dir->d_ino, dir->d_name);
	return buf;
}

static u64 root_dir (tree_s *tree)
{
	return tree->t_root;
}

static int change_root_dir (tree_s *tree, void *root)
{
	hub_s	*hub;
FN;
	tree->t_root = tau_bnum(root);

	hub = container_of(tree, hub_s, h_dir);
	hub->h_dirty = TRUE;
	return 0;
}

