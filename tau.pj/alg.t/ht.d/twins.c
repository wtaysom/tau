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

#include "crfs.h"
#include "ht.h"
#include "twins.h"

Htree_s	*TreeA;
Htree_s	*TreeB;

int delete_twins (Key_t key)
{
	int	rc;

	rc = t_delete(TreeA, key);
	if (rc) {
		printf("Failed delete key for TreeA \"%llx\" err=%d\n",
			(u64)key, rc);
	}
	rc = t_delete(TreeB, key);
	if (rc) {
		printf("Failed delete key for TreeB \"%llx\" err=%d\n",
			(u64)key, rc);
	}
	return rc;
}

int insert_twins (Key_t key)
{
	int	rc;

	rc = t_insert(TreeA, key);
	if (rc) {
		printf("TreeA failed to insert string \"%llx\" err=%d\n",
			(u64)key, rc);
		return rc;
	}
	rc = t_insert(TreeB, key);
	if (rc) {
		printf("TreeB failed to insert string \"%llx\" err=%d\n",
			(u64)key, rc);
		return rc;
	}
	return rc;
}

int find_twins (Key_t key)
{
	int	rc_a;
	int	rc_b;

	rc_a = t_find(TreeA, key);
	rc_b = t_find(TreeB, key);

	if (rc_a != rc_b) {
		printf("find_twins \"%llx\" %d!=%d\n", (u64)key, rc_a, rc_b);
	}
	return rc_a;
}

int next_twins (
	u64	key,
	u64	*next_key)
{
	u64	next_a;
	u64	next_b;
	int	rc_a;
	int	rc_b;

	rc_a = next_string(TreeA, key, &next_a, name_a);
	rc_b = next_string(TreeB, key, &next_b, name_b);

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


void dump_string (Htree_s *tree, u64 rec_key, void *rec, unint len)
{
	char	*s = rec;

	printf("%s", s);
}

u64 root_string (Htree_s *tree)
{
	super_s	*super;
	buf_s	*buf;
	u64	root;
FN;
	buf = bget(tree->t_dev, SUPER_BLOCK);
	if (!buf) {
		eprintf("root_string: no super block");
		return HT_ERR_NOT_FOUND;
	}
	super = buf->b_data;
	root = super->sp_root;
	bput(buf);
	return root;
}

int change_root_string (Htree_s *tree, buf_s *root)
{
	super_s	*super;
	buf_s	*buf;
FN;
	buf = bget(tree->t_dev, SUPER_BLOCK);
	if (!buf) {
		eprintf("change_root_string: no super block");
		return HT_ERR_NOT_FOUND;
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

static void init_super_block (Htree_s *tree)
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

void init_string (Htree_s *tree, log_s *log)
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
