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

//===================================================================
static char *dump_shard      (tree_s *tree, u64 rec_key, void *rec, unint size);
static u64 root_shard        (tree_s *tree);
static int change_root_shard (tree_s *tree, void *root);
static int pack_shard (void *to, void *from, unint size);

tree_species_s	Tau_shard_species = {
	"Shard",
	ts_dump:	dump_shard,
	ts_root:	root_shard,
	ts_change_root:	change_root_shard,
	ts_pack:	pack_shard};

//===================================================================

static inline u64 shard_to_key (tau_shard_s *tau_shard)
{
	u64	key = (((u64)tau_shard->ts_shard_no) << 32)
			| tau_shard->ts_replica_no;
	return key;
}

//===================================================================

typedef struct delete_search_s {
	u64		key;
} delete_search_s;

static int delete_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	delete_search_s	*s = data;
FN;
	if (s->key == rec_key) {
		return 0;
	}
	if (s->key < rec_key) {
		return qERR_NOT_FOUND;
	}
	return qERR_TRY_NEXT;
}

int tau_delete_shard (tree_s *ptree, tau_shard_s *tau_shard)
{
	delete_search_s		s;
	int	rc;
FN;
	s.key = shard_to_key(tau_shard);
	rc = tau_search(ptree, s.key, delete_search, &s);
	if (rc) {
		dprintk("delete_shard [%d] couldn't find %d.%d\n",
			rc, tau_shard->ts_shard_no, tau_shard->ts_replica_no);
		return rc;
	}
	rc = tau_delete(ptree, s.key);
	if (rc) {
		eprintk("Failed to delete shard %d.%d err=%d\n",
			tau_shard->ts_shard_no, tau_shard->ts_replica_no, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct insert_search_s {
	u64		key;
	tau_shard_s	*tau_shard;
} insert_search_s;

static int insert_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	insert_search_s	*s = data;
FN;
	if (s->key < rec_key) {
		return qERR_NOT_FOUND;
	}
	return qERR_TRY_NEXT;
}

static int pack_shard (void *to, void *from, unint size)
{
	insert_search_s	*s = from;
	tau_shard_s	*tau_shard = to;
FN;
	*tau_shard = *s->tau_shard;
	return 0;
}

int tau_insert_shard (tree_s *ptree, tau_shard_s *tau_shard)
{
	insert_search_s	s;
	unint		size;
	int		rc;
FN;
	s.key = shard_to_key(tau_shard);
	s.tau_shard = tau_shard;
	size = sizeof(*tau_shard);
	rc = tau_insert(ptree, s.key, &s, size);
	if (rc == 0) return 0;

	if (rc != qERR_DUP) {
		eprintk("Failed to insert shard %d.%d err=%d\n",
			tau_shard->ts_shard_no, tau_shard->ts_replica_no, rc);
		return rc;
	}

	rc = tau_search(ptree, s.key, insert_search, &s);
	if (rc == qERR_DUP) {
		dprintk("Duplicate shard found %d.%d err=%d\n",
			tau_shard->ts_shard_no, tau_shard->ts_replica_no, rc);
		return rc;
	}
	rc = tau_insert(ptree, s.key, &s, size);
	if (rc) {
		eprintk("Failed to insert shard %d.%d err=%d\n",
			tau_shard->ts_shard_no, tau_shard->ts_replica_no, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct find_search_s {
	u64		key;
	tau_shard_s	*tau_shard;
} find_search_s;

static int find_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	find_search_s	*s = data;
	tau_shard_s	*tau_shard = rec;
FN;
	if (s->key == rec_key) {
		*s->tau_shard = *tau_shard;
		return 0;
	}
	if (s->key < rec_key) {
		return qERR_NOT_FOUND;
	}
	return qERR_TRY_NEXT;
}

int tau_find_shard (tree_s *ptree, tau_shard_s *tau_shard)
{
	find_search_s	s;
	int		rc;
FN;
	s.key = shard_to_key(tau_shard);
	s.tau_shard = tau_shard;

	rc = tau_search(ptree, s.key, find_search, &s);
	return rc;
}

//===================================================================

static char *dump_shard (tree_s *tree, u64 rec_key, void *rec, unint size)
{
	static char	buf[4096];
	tau_shard_s	*tau_shard = rec;
FN;
	snprintf(buf, sizeof(buf), "%d.%d\n",
		tau_shard->ts_shard_no, tau_shard->ts_replica_no);
	return buf;
}

static u64 root_shard (tree_s *tree)
{
	return tree->t_root;
}

static int change_root_shard (tree_s *tree, void *root)
{
	bag_s	*bag;
	u64		blkno;
FN;
	blkno = tau_bnum(root);
	tree->t_root = blkno;

	bag = container_of(tree, bag_s, bag_tree);
	bag->bag_block->tb_shard_btree = blkno;
	tau_blog(bag->bag_block);
	return 0;
}

