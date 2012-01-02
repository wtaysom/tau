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

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <ht.h>
#include <twins.h>

extern tree_species_s	String_species;

Htree_s	*TreeA;
Htree_s	*TreeB;

enum {	UNIQUE_SHIFT = 8,
	UNIQUE_MASK = (1 << UNIQUE_SHIFT) - 1,
	HASH_MASK = ~UNIQUE_MASK};

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

int delete_string (tree_s *tree, char *name)
{
	delete_search_s	s;
	int	rc;

	s.key = hash_string(name);
	s.name = name;
	rc = search(tree, s.key, delete_search, &s);
	if (rc) {
		printf("delete_string [%d] couldn't find %s\n", rc, name);
		return rc;
	}
	rc = delete(tree, s.key);
	if (rc) {
		printf("Failed to delete string \"%s\" err=%d\n",
			name, rc);
	}
	return 0;
}

int delete_twins (char *name)
{
	int	rc;

	rc = delete_string( &TreeA, name);
	if (rc) {
		printf("Failed delete string for TreeA \"%s\" err=%d\n",
			name, rc);
	}
	rc = delete_string( &TreeB, name);
	if (rc) {
		printf("Failed delete string for TreeB \"%s\" err=%d\n",
			name, rc);
	}
	return rc;
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

int insert_string (tree_s *tree, char *name)
{
	insert_search_s	s;
	unint	len;
	int	rc;

	len = strlen(name) + 1;
	s.key = hash_string(name);
	rc = insert(tree, s.key, name, len);
	if (rc == 0) {
		return 0;
	}
	if (rc != qERR_DUP) {
		printf("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}

	s.name = name;
	s.set_new_key = FALSE;

	rc = search(tree, s.key, insert_search, &s);
	if (rc == qERR_DUP) {
		printf("Duplicate string found \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	rc = insert(tree, s.key, name, len);
	if (rc) {
		printf("Failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	return 0;
}

int insert_twins (char *name)
{
	int	rc;

	rc = insert_string( &TreeA, name);
	if (rc) {
		printf("TreeA failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	rc = insert_string( &TreeB, name);
	if (rc) {
		printf("TreeB failed to insert string \"%s\" err=%d\n",
			name, rc);
		return rc;
	}
	return rc;
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

int find_string (tree_s *tree, char *name)
{
	int		rc;
	find_search_s	s;

	s.key = hash_string(name);
	s.name = name;

	rc = search(tree, s.key, find_search, &s);
	return rc;
}

int find_twins (char *name)
{
	int	rc_a;
	int	rc_b;

	rc_a = find_string( &TreeA, name);
	rc_b = find_string( &TreeB, name);

	if (rc_a != rc_b) {
		printf("find_twins \"%s\" %d!=%d\n", name, rc_a, rc_b);
	}
	return rc_a;
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
	tree_s	*tree,
	u64	key,
	u64	*next_key,
	char	*name)
{
	next_search_s	s;
	int		rc;

	s.key = key;
	s.name = name;
	rc = search(tree, s.key, next_search, &s);
	if (!rc) {
		*next_key = s.key;
	}
	return rc;
}

int next_twins (
	u64	key,
	u64	*next_key,
	char	*name)
{
	char	name_a[MAX_NAME+1];
	char	name_b[MAX_NAME+1];
	u64	next_a;
	u64	next_b;
	int	rc_a;
	int	rc_b;

	rc_a = next_string( &TreeA, key, &next_a, name_a);
	rc_b = next_string( &TreeB, key, &next_b, name_b);

	if (rc_a && (rc_a == rc_b)) {
		return rc_a;
	}
	if ((rc_a != rc_b)
		|| (next_a != next_b)
		|| (strcmp(name_a, name_b) != 0))
	{
		printf("Failed next_twins %d ? %d %llx ? %llx %s ? %s\n",
			rc_a, rc_b, next_a, next_b, name_a, name_b);
	}
	if (!rc_a) {
		*next_key = next_a;
		strcpy(name, name_a);
	}
	return rc_a;
}


//===================================================================

enum {	SUPER_BLOCK = 1,
	SUPER_MAGIC = 0x11111111 };

typedef struct super_s {
	__u64	sp_blknum;
	__u32	sp_magic;
	__u32	sp_reserved;
	__u64	sp_root;
	__u64	sp_next;
} super_s;


void dump_string (tree_s *tree, u64 rec_key, void *rec, unint len)
{
	char	*s = rec;

	printf("%s", s);
}

u64 root_string (tree_s *tree)
{
	super_s	*super;
	buf_s	*buf;
	u64	root;
FN;
	buf = bget(tree->t_dev, SUPER_BLOCK);
	if (!buf) {
		eprintf("root_string: no super block");
		return qERR_NOT_FOUND;
	}
	super = buf->b_data;
	root = super->sp_root;
	bput(buf);
	return root;
}

int change_root_string (tree_s *tree, buf_s *root)
{
	super_s	*super;
	buf_s	*buf;
FN;
	buf = bget(tree->t_dev, SUPER_BLOCK);
	if (!buf) {
		eprintf("change_root_string: no super block");
		return qERR_NOT_FOUND;
	}
	super = buf->b_data;
	super->sp_root = root->b_blknum;
	bdirty(buf);
	bput(buf);
	return 0;
}

buf_s *alloc_block (dev_s *dev)
{
	super_s	*super;
	buf_s	*buf;
	u64	blknum;
FN;
	buf = bget(dev, SUPER_BLOCK);
	super = buf->b_data;

	if (super->sp_magic != SUPER_MAGIC) {
		eprintf("no super block");
	}
	blknum = super->sp_next++;
	bdirty(buf);
	bput(buf);

	return bnew(dev, blknum);
}

static void init_super_block (tree_s *tree)
{
	super_s	*super;
	buf_s	*buf;

	buf = bget(tree->t_dev, SUPER_BLOCK);
	if (!buf) {
		buf = bnew(tree->t_dev, SUPER_BLOCK);
	}
	super = buf->b_data;
	if (super->sp_magic != SUPER_MAGIC) {
		super->sp_blknum = SUPER_BLOCK;
		super->sp_magic  = SUPER_MAGIC;
		super->sp_root   = 0;
		super->sp_next   = SUPER_BLOCK + 1;

		bdirty(buf);
	}
	bput(buf);
}

void init_string (tree_s *tree, log_s *log)
{
	init_tree(tree, &String_species, log->lg_sys, log);

	init_super_block(tree);
}

void init_twins (char *dev_A, char *log_A, char *dev_B, char *log_B)
{
	static log_s	LogA;
	static log_s	LogB;

	init_log( &LogA, log_A, dev_A);
	init_log( &LogB, log_B, dev_B);

	init_string( &TreeA, &LogA);
	init_string( &TreeB, &LogB);
	blazy(TreeA.t_dev);
}

//===================================================================

tree_species_s	String_species = {
	"String",
	ts_dump:	dump_string,
	ts_root:	root_string,
	ts_change_root:	change_root_string,
	ts_pack:	pack_string};

