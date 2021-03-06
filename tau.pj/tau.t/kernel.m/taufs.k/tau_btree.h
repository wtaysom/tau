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

#ifndef _TAU_BTREE_H_
#define _TAU_BTREE_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct tree_s	tree_s;

typedef int	(*search_f)(
			void	*data,
			u64	rec_key,
			void	*rec,
			unint	size);

typedef struct tree_species_s {
	char	*ts_name;
	char	*(*ts_dump)       (tree_s *tree, u64 rec_key, void *rec, unint size);
	u64	(*ts_root)        (tree_s *tree);
	int	(*ts_change_root) (tree_s *tree, void *root);
	int	(*ts_pack)        (void *to, void *from, unint size);
} tree_species_s;

struct tree_s {
	tree_species_s	*t_species;
	struct inode	*t_inode;
	u64		t_root;
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
	struct inode	*inode,
	u64		root_blknum)
{
	tree->t_species = species;
	tree->t_inode   = inode;	//XXX: Shouldn't this be knode?
	tree->t_root    = root_blknum;
}

static inline char* dump_rec (
	tree_s	*tree,
	u64	rec_key,
	void	*rec,
	unint	size)
{
	if (tree->t_species->ts_dump) {
		return tree->t_species->ts_dump(tree, rec_key, rec, size);
	} else {
		return "";
	}
}

static inline addr root (tree_s *tree)
{
	return tree->t_species->ts_root(tree);
}

static inline int change_root (tree_s *tree, void *root)
{
	return tree->t_species->ts_change_root(tree, root);
}

static inline void pack_rec (tree_s *tree, void *to, void *from, unint size)
{
	tree->t_species->ts_pack(to, from, size);
}

void tau_dump_tree (tree_s *tree);

int tau_insert (
	tree_s	*tree,
	u64	key,
	void	*rec,
	unint	size);

int tau_delete (
	tree_s	*tree,
	u64	key);

int tau_search (
	tree_s		*tree,
	u64		key,
	search_f	sf,
	void		*data);

void tau_stat_tree (
	tree_s		*tree,
	btree_stat_s	*bs);

int tau_compare_trees (tree_s *a, tree_s *b);

#endif
