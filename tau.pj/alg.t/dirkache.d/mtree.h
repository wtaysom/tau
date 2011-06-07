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

#ifndef _MTREE_H_
#define _MTREE_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#define xprintf(L, fmt, arg...) \
	printf( # L ": %s<%d>: " fmt "\n", __func__, __LINE__, ## arg)

#define iprintf(fmt, arg...) xprintk(Info, fmt, ## arg)
#define dprintf(fmt, arg...) xprintf(Debug, fmt, ## arg)


enum { PAGE_SIZE = 128 };

typedef enum {
	qERR_FULL          = -1000,
	qERR_BAD_TREE      = -1001,
	qERR_DUP           = -1002,
	qERR_NOT_FOUND     = -1003,
	qERR_TRY_NEXT      = -1004,
	qERR_HASH_OVERFLOW = -1005,
	qERR_BAD_BLOCK     = -1006
} err_t;

typedef struct tree_s	tree_s;

typedef int	(*search_f)(
			void	*data,
			u32	rec_key,
			void	*rec,
			unint	size);

typedef struct tree_species_s {
	char	*ts_name;
	char	*(*ts_dump)       (tree_s *tree, u32 rec_key, void *rec, unint size);
	void	*(*ts_root)       (tree_s *tree);
	int	(*ts_change_root) (tree_s *tree, void *root);
	int	(*ts_pack)        (void *to, void *from, unint size);
} tree_species_s;

struct tree_s {
	tree_species_s	*t_species;
	void		*t_root;
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
	tree_species_s	*species)
{
	tree->t_species = species;
	tree->t_root    = NULL;
}

static inline char* dump_rec (
	tree_s	*tree,
	u32	rec_key,
	void	*rec,
	unint	size)
{
	if (tree->t_species->ts_dump) {
		return tree->t_species->ts_dump(tree, rec_key, rec, size);
	} else {
		return "";
	}
}

static inline void *root (tree_s *tree)
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

void dump_tree (tree_s *tree);

int insert (
	tree_s	*tree,
	u32	key,
	void	*rec,
	unint	size);

int delete (
	tree_s	*tree,
	u32	key);

int search (
	tree_s		*tree,
	u32		key,
	search_f	sf,
	void		*data);

void stat_tree (
	tree_s		*tree,
	btree_stat_s	*bs);

int compare_trees (tree_s *a, tree_s *b);

#endif
