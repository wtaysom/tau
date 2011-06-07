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

#ifndef _BTREE_H_
#define _BTREE_H_

#ifndef _BLK_H_
#include <blk.h>
#endif

#ifndef _LOG_H_
#include <log.h>
#endif

typedef struct tree_s	tree_s;

typedef int	(*search_f)(
			void	*data,
			u64	rec_key,
			void	*rec,
			unint	len);

typedef struct tree_species_s {
	char	*ts_name;
	void	(*ts_dump)        (tree_s *tree, u64 rec_key, void *rec, unint len);
	u64	(*ts_root)        (tree_s *tree);
	int	(*ts_change_root) (tree_s *tree, buf_s *root);
	int	(*ts_pack)        (void *to, void *from, unint len);
} tree_species_s;

struct tree_s {
	tree_species_s	*t_species;
	dev_s		*t_dev;
	log_s		*t_log;
	u8		t_slot;
};

typedef struct btree_stat_s {
	u64	st_blk_size;
	u64	st_num_leaves;
	u64	st_num_branches;
	u64	st_leaf_free_space;
	u64	st_leaf_recs;
	u64	st_branch_recs;
	u64	st_height;
} btree_stat_s;

static inline void init_tree (
	tree_s		*tree,
	tree_species_s	*species,
	dev_s		*dev,
	log_s		*log,
	u8		slot)
{
	tree->t_species = species;
	tree->t_dev     = dev;
	tree->t_log     = log;
	tree->t_slot    = slot;
}

static inline void dump_rec (tree_s *tree, u64 rec_key, void *rec, unint len)
{
	if (tree->t_species->ts_dump) {
		tree->t_species->ts_dump(tree, rec_key, rec, len);
	}
}

static inline u64 root (tree_s *tree)
{
	return tree->t_species->ts_root(tree);
}

static inline int change_root (tree_s *tree, buf_s *root)
{
	return tree->t_species->ts_change_root(tree, root);
}

static inline void pack_rec (tree_s *tree, void *to, void *from, unint len)
{
	tree->t_species->ts_pack(to, from, len);
}

void dump_tree (tree_s *tree);

int insert (
	tree_s	*tree,
	u64	key,
	void	*rec,
	unint	len);

int delete (
	tree_s	*tree,
	u64	key);

int search (
	tree_s		*tree,
	u64		key,
	search_f	sf,
	void		*data);

void stat_tree (
	tree_s		*tree,
	btree_stat_s	*bs);

int compare_trees (tree_s *a, tree_s *b);


int bind_tree_log (log_s *log, u8 slot);

#endif
