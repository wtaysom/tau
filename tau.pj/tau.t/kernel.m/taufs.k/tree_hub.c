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
#include <crc.h>
#include <tau/debug.h>
#include <tau/msg.h>
#include <util.h>
#include <tau_err.h>
#include <tau_blk.h>
#include <tau_fs.h>
#include <kache.h>

// Could put type info in bottom bits and top bits
// 
enum {	TYPE_SHIFT = 56,
	VALUE_MASK = (1LL << TYPE_SHIFT) - 1 };

enum inode_types {
	TYPE_INODE  = 1LL << TYPE_SHIFT,
	TYPE_EXTENT = 2LL << TYPE_SHIFT };

//===================================================================
static char *dump_hub      (tree_s *tree, u64 rec_key, void *rec, unint size);
static u64 root_hub        (tree_s *tree);
static int change_root_hub (tree_s *tree, void *root);
static int pack_hub (void *to, void *from, unint size);

tree_species_s	Tau_hub_species = {
	"Inode",
	ts_dump:	dump_hub,
	ts_root:	root_hub,
	ts_change_root:	change_root_hub,
	ts_pack:	pack_hub};

hub_s *alloc_hub (unint name_len)
{
	hub_s	*hub;
FN;
	hub = zalloc_tau(sizeof(*hub) + name_len);
	if (!hub) {
		eprintk("Failed to allocate hub with name len %ld", name_len);
		return NULL;
	}
	hub->h_tnode.t_name_len = name_len;
	init_dq( &hub->h_spokes);
	return hub;
}

void free_hub (hub_s *hub)
{
FN;
	assert(is_empty_dq( &hub->h_spokes));
	free_tau(hub);
}

static int pack_hub (void *to, void *from, unint size)
{
	tau_inode_s	*tnode = to;
	hub_s	*hub = from;
FN;
	*tnode = hub->h_tnode;
	strncpy(tnode->t_name, hub->h_tnode.t_name, tnode->t_name_len);
	zero(tnode->t_reserved);

	return 0;
}

static hub_s *unpack_hub (tau_inode_s *tnode)
{
	hub_s	*hub;
FN;
	hub = alloc_hub(tnode->t_name_len);
	if (!hub) return NULL;

	hub->h_tnode = *tnode;
	strncpy(hub->h_tnode.t_name, tnode->t_name, tnode->t_name_len);

	return hub;
}

//===================================================================

static inline u64 ino_to_key (u64 ino)
{
	return ino | TYPE_INODE;
}

//===================================================================

typedef struct delete_search_s {
	u64	key;
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

int tau_delete_hub (tree_s *ptree, u64 ino)
{
	delete_search_s		s;
	int	rc;
FN;
	s.key = ino_to_key(ino);
	rc = tau_search(ptree, s.key, delete_search, &s);
	if (rc) {
		dprintk("delete_hub [%d] couldn't find %lld\n", rc, ino);
		return rc;
	}
	rc = tau_delete(ptree, s.key);
	if (rc) {
		eprintk("Failed to delete inode %lld err=%d\n",
			ino, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct insert_search_s {
	u64		key;
	hub_s	*hub;
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

//XXX: hub should have pointer to tree in it (or at least the shard where said
// tree resides!
int tau_insert_hub (tree_s *ptree, hub_s *hub)
{
	insert_search_s	s;
	tau_inode_s	*t = &hub->h_tnode;
	u64		ino = t->t_ino;
	unint		size;
	int		rc;
	unint		name_len;

	s.key = ino_to_key(ino);
	s.hub = hub;
	name_len = strlen(t->t_name) + 1;
	size = name_len + sizeof(*t);
	rc = tau_insert(ptree, s.key, &s, size);
	if (rc == 0) return 0;

	if (rc != qERR_DUP) {
		eprintk("Failed to insert inode %lld err=%d\n",
			ino, rc);
		return rc;
	}

	rc = tau_search(ptree, s.key, insert_search, &s);
	if (rc == qERR_DUP) {
		dprintk("Duplicate inode found %lld err=%d\n",
			ino, rc);
		return rc;
	}
	rc = tau_insert(ptree, s.key, &s, size);
	if (rc) {
		eprintk("Failed to insert inode %lld err=%d\n",
			ino, rc);
		return rc;
	}
	return 0;
}

//===================================================================

typedef struct find_search_s {
	u64	key;
	hub_s	*hub;
} find_search_s;

static int find_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	find_search_s	*s = data;
FN;
	if (s->key == rec_key) {
		s->hub = unpack_hub(rec);
		if (!s->hub) return ENOMEM;
		return 0;
	}
	if (s->key < rec_key) {
		return qERR_NOT_FOUND;
	}
	return qERR_TRY_NEXT;
}

int tau_find_hub (tree_s *ptree, u64 ino, hub_s **phub)
{
	find_search_s	s;
	int		rc;
FN;
	s.key   = ino_to_key(ino);
	s.hub = NULL;

	rc = tau_search(ptree, s.key, find_search, &s);
	if (rc) return rc;
	init_tree( &s.hub->h_dir, &Tau_dir_species,
			ptree->t_inode, s.hub->h_tnode.t_btree_root);
	*phub = s.hub;
	return 0;
}

//===================================================================

static char *dump_hub (tree_s *tree, u64 rec_key, void *rec, unint size)
{
	static char	buf[4096];

	tau_inode_s	*tnode = rec;

	snprintf(buf, sizeof(buf), "%lld %s", tnode->t_ino, tnode->t_name);
	return buf;
}

static u64 root_hub (tree_s *tree)
{
	return tree->t_root;
}

static int change_root_hub (tree_s *tree, void *root)
{
	shard_s		*shard  = container_of(tree, shard_s, sh_inodes);
	tau_shard_s	*tshard = &shard->sh_disk;
	int		rc;
FN;
	tree->t_root = tau_bnum(root);

	rc = tau_delete_shard( &shard->sh_bag->bag_tree, tshard);
	if (rc) {
		eprintk("tau_delete_shard %d", rc);
		return rc;
	}
	tshard->ts_root = tree->t_root;
	rc = tau_insert_shard( &shard->sh_bag->bag_tree, tshard);
	if (rc) {
		eprintk("tau_insert_shard %d", rc);
		return rc;
	}
	return 0;
}
