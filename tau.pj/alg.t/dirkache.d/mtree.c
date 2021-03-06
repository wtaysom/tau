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
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <mtree.h>

/*
 * 1. Noticed that I can get a lot of empty leaves
 *	when a leaf goes to 0 force join
 * 2. Need to write shrink, even if we never use it
 * 3. Insert with merge
 *	a. right merge is easy
 *	b. merge left is a little more difficult because the b-tree algorithm
 *		is not sysmetrical.  Moving to the right, is easier than
 *		moving left because we use < for key comparison.
 * 4. Browse:
 *	a. Do they want to be able to modify records that are being browsed?
 *	b. How will they know the structure of the data (browse by type?)
 * 5. If I link siblings for both branches and leaves, I could have a
 *	btree that just needs ordered writes and no logging to maintain
 *	consistency.  Split order: sibling, child (with link to sibling),
 *	parent.  I think reversing the order on join, parent, child, free
 *	sibling.  May still need something to keep from losing block
 *	allocated for a sibling.
 *
 * Theory of Operation: (only notes so far)
 */

enum { MIN_HOLE = 2 };

typedef enum {	LEAF = 0x1eaf1eaf,
		BRANCH = 0xbac4bac4 } block_t;

typedef struct key_s {
	u32	k_key;
	void	*k_node;
} key_s;

typedef struct rec_s {
	s16	r_start;
	s16	r_size;
	u32	r_key;
} rec_s;

#define HEAD_S struct {		\
	void	*h_node;	\
	u32	h_magic;	\
}

typedef struct head_s {
	HEAD_S;
} head_s;

/* b0 k1 b1 k2 b2 k3 b3 */
typedef struct branch_s {
	HEAD_S;
	s16	br_num;
	u16	br_reserved[3];
	void	*br_first;
	key_s	br_key[0];
} branch_s;

typedef struct leaf_s {
	HEAD_S;
	s16	l_num;
	s16	l_end;
	s16	l_total;	// change to free later
	u16	l_reserved[1];
	// Another way to make end work would be to use
	// &l_rec[l_num] as the index point of l_end
	// That is what I did in insert!!!
	rec_s	l_rec[0];
} leaf_s;

enum {
	KEYS_PER_BRANCH = (PAGE_SIZE - sizeof(branch_s)) / sizeof(key_s),
	MAX_FREE = PAGE_SIZE - sizeof(leaf_s)
};

void dump_node (
	tree_s	*tree,
	void	*node,
	unint	indent);

char *indent (unint i)
{
	static char blanks[] = "                          ";

	i *= 4;

	if (i >= sizeof(blanks)) return blanks;
	return &blanks[sizeof(blanks) - 1 - i];
}

void pr_leaf (leaf_s *leaf)
{
	dprintf("LEAF block=%p num=%x end=%x total=%x\n",
		leaf->h_node, leaf->l_num, leaf->l_end, leaf->l_total);
}

static inline u32 magic (void *head)
{
	head_s	*h = head;

	return h->h_magic;
}

static void *alloc_node (void)
{
	void	*p;
	int	rc;
FN;
	rc = posix_memalign( &p, PAGE_SIZE, PAGE_SIZE);
	if (rc) {
		eprintf("posix_memalign:");
	}
	return p;
}

/*
 *	+--------+------+------------+------------------+
 *	| leaf_s | recs | free space | value of records |
 *	+--------+------+------------+------------------+
 *	         ^      ^            ^
 *	         |l_num |           l_end
 */
static inline snint free_space (leaf_s *leaf)
{
	return leaf->l_end - (leaf->l_num * sizeof(rec_s) + sizeof(leaf_s));
}

void dump_leaf (
	tree_s	*tree,
	leaf_s	*leaf,
	unint	depth)
{
	rec_s	*end = &leaf->l_rec[leaf->l_num];
	rec_s	*r;
	char	*c;
	unint	i;

	if (!leaf) return;

	printf("%sleaf: num keys = %d end = %d total = %d current = %ld\n",
			indent(depth),
			leaf->l_num, leaf->l_end, leaf->l_total,
			free_space(leaf));
	for (r = leaf->l_rec, i = 0; r < end; r++, i++) {
		c = (char *)leaf + r->r_start;
		printf("%s    %4lu. %8x %4d:%4d %s\n",
			indent(depth),
			i, r->r_key, r->r_start, r->r_size,
			dump_rec(tree, r->r_key, c, r->r_size));

	}
}

void dump_branch (
	tree_s		*tree,
	branch_s	*branch,
	unint		depth)
{
	key_s	*k;
	unint	i;

	if (!branch) return;

	printf("%sbranch: num keys = %d\n",
		indent(depth), branch->br_num);
	printf("%sfirst %p\n", indent(depth), branch->br_first);
	dump_node(tree, branch->br_first, depth+1);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		printf("%s%4lu. %8x: %p\n",
			indent(depth),
			i, k->k_key, k->k_node);
		dump_node(tree, k->k_node, depth+1);
	}
}

void dump_node (
	tree_s	*tree,
	void	*node,
	unint	depth)
{
	head_s	*h = node;

	printf("node=%p\n", node);
	switch (h->h_magic) {
	case LEAF:	dump_leaf(tree, (leaf_s *)h, depth);
			break;
	case BRANCH:	dump_branch(tree, (branch_s *)h, depth);
			break;
	default:	prmem("Unknown block magic", h, PAGE_SIZE);
			break;
	}
}

void dump_tree (tree_s *tree)
{
	printf("Tree------------------------------------------------\n");
	dump_node(tree, root(tree), 0);
}

void verify_leaf (tree_s *tree, leaf_s *leaf, char *where)
{
	snint	sum;
	unint	i;

	if (leaf->h_magic != LEAF) {
		eprintf("ERROR:%s leaf=%p magic=%x\n",
			where, leaf, leaf->h_magic);
	}
	if (leaf->l_total < free_space(leaf)) {
		eprintf("ERROR:%s allocation space bigger than total\n",
			where);
		dump_leaf(tree, leaf, 0);
	}
	if (free_space(leaf) < 0) {
		eprintf("ERROR:%s leaf overflow\n", where);
		dump_leaf(tree, leaf, 0);
	}
	sum = sizeof(leaf_s) + (leaf->l_num * sizeof(rec_s));
	for (i = 0; i < leaf->l_num; i++) {
		sum += leaf->l_rec[i].r_size;
	}
	if (PAGE_SIZE - sum != leaf->l_total) {
		eprintf("ERROR:%s totals %ld != %d\n", where,
			PAGE_SIZE - sum, leaf->l_total);
		dump_leaf(tree, leaf, 0);
	}
	for (i = 1; i < leaf->l_num; i++) {
		if (leaf->l_rec[i-1].r_key >= leaf->l_rec[i].r_key) {
			eprintf("ERROR:%s keys out of order %llx >= %llx\n",
				where,
				leaf->l_rec[i-1].r_key, leaf->l_rec[i].r_key);
			dump_leaf(tree, leaf, 0);
		}
	}

}

leaf_s *new_leaf (tree_s *tree)
{
	leaf_s	*leaf;
FN;
	leaf = alloc_node();
	leaf->h_node = leaf;
	leaf->h_magic = LEAF;
	leaf->l_end   = PAGE_SIZE;
	leaf->l_total = MAX_FREE;
	return leaf;
}

branch_s *new_branch (tree_s *tree)
{
	branch_s	*branch;
FN;
	branch = alloc_node();
	branch->h_node = branch;
	branch->h_magic = BRANCH;
	return branch;
}

void stat_node (
	void		*head,
	btree_stat_s	*bs,
	unint		height);

void stat_leaf (leaf_s *leaf, btree_stat_s *bs)
{
	++bs->st_num_leaves;
	bs->st_leaf_free_space += leaf->l_total;
	bs->st_leaf_recs       += leaf->l_num;
}

void stat_branch (
	branch_s	*branch,
	btree_stat_s	*bs,
	unint		height)
{
	key_s	*k;
	unint	i;

	++bs->st_num_branches;
	bs->st_branch_recs += branch->br_num;

	stat_node(branch->br_first, bs, height);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		stat_node(k->k_node, bs, height);
	}
}

void stat_node (
	void		*node,
	btree_stat_s	*bs,
	unint		height)
{
	head_s	*h = node;

	if (!h) return;

	switch (h->h_magic) {
	case LEAF:	stat_leaf(node, bs);
			break;
	case BRANCH:	stat_branch(node, bs, height + 1);
			break;
	default:	eprintf("Unknown block magic %x\n", h->h_magic);
			break;
	}
	if (height > bs->st_height) {
		bs->st_height = height;
	}
}

void stat_tree (tree_s *tree, btree_stat_s *bs)
{
	zero(*bs);
	bs->st_blk_size = PAGE_SIZE;
	stat_node(root(tree), bs, 1);
}

static int compare_nodes (tree_s *a, tree_s *b, head_s *ahead, head_s *bhead);

static int compare_rec (rec_s *a, rec_s *b)
{
	if (a->r_key != b->r_key) {
		eprintf("rec key doesn't match %llx!=%llx\n",
			a->r_key, b->r_key);
		return qERR_BAD_TREE;
	}
	if (a->r_start != b->r_start) {
		eprintf("rec start doesn't match %x!=%x\n",
			a->r_start, b->r_start);
		return qERR_BAD_TREE;
	}
	if (a->r_size != b->r_size) {
		eprintf("rec size doesn't match %x!=%x\n",
			a->r_size, b->r_size);
		return qERR_BAD_TREE;
	}
	return 0;
}

static int compare_leaves (tree_s *a, tree_s *b, leaf_s *aleaf, leaf_s *bleaf)
{
	unint	i;
	int	rc;

	if (aleaf->l_num != bleaf->l_num) {
		eprintf("leaf num doesn't match %x!=%x\n",
			aleaf->l_num, bleaf->l_num);
		return qERR_BAD_TREE;
	}
	if (aleaf->l_end != bleaf->l_end) {
		eprintf("leaf end doesn't match %x!=%x\n",
			aleaf->l_end, bleaf->l_end);
		return qERR_BAD_TREE;
	}
	if (aleaf->l_total != bleaf->l_total) {
		eprintf("leaf total doesn't match %x!=%x\n",
			aleaf->l_total, bleaf->l_total);
		return qERR_BAD_TREE;
	}
	for (i = 0; i < aleaf->l_num; i++) {
		rc = compare_rec( &aleaf->l_rec[i], &bleaf->l_rec[i]);
		if (rc) return rc;
	}
	return 0;
}

static int compare_branches (
	tree_s		*a,
	tree_s		*b,
	branch_s	*abranch,
	branch_s 	*bbranch)
{
	key_s	*akey;
	key_s	*bkey;
	unint	i;
	int	rc;

	if (abranch->br_num != bbranch->br_num) {
		eprintf("branch num doesn't match %x!=%x\n",
			abranch->br_num, bbranch->br_num);
		return qERR_BAD_TREE;
	}
	rc = compare_nodes(a, b, abranch->br_first, bbranch->br_first);
	if (rc) return rc;
	for (i = 0; i < abranch->br_num; i++) {
		akey = &abranch->br_key[i];
		bkey = &bbranch->br_key[i];
		if (akey->k_key != bkey->k_key) {
			eprintf("branch key %lu doesn't match %llx!=%llx\n",
				i, akey->k_key, bkey->k_key);
			return qERR_BAD_TREE;
		}
		if (akey->k_node != bkey->k_node) {
			eprintf("key block %lu doesn't match %llx!=%llx\n",
				i, akey->k_node, bkey->k_node);
			return qERR_BAD_TREE;
		}
		rc = compare_nodes(a, b, akey->k_node, bkey->k_node);
		if (rc) return rc;
	}
	return 0;
}

static int compare_nodes (tree_s *a, tree_s *b, head_s *ahead, head_s *bhead)
{
	int	rc;

	if (!ahead) {
		if (bhead) return qERR_BAD_TREE;
		else return 0;
	} else if (!bhead) return qERR_BAD_TREE;

	if (memcmp(ahead, bhead, PAGE_SIZE) != 0) {
		eprintf("a and b are not binary identical\n");
	}

	if (ahead->h_magic != bhead->h_magic) {
		eprintf("Magic doesn't match %x!=%x\n",
			ahead->h_magic, bhead->h_magic);
		return qERR_BAD_TREE;
	}
	if (ahead->h_node != bhead->h_node) {
		eprintf("Blknum doesn't match %llx!=%llx\n",
			ahead->h_node, bhead->h_node);
		return qERR_BAD_TREE;
	}
	switch (ahead->h_magic) {
	case LEAF:	rc = compare_leaves(a, b,
					(leaf_s *)ahead, (leaf_s *)bhead);
			break;
	case BRANCH:	rc = compare_branches(a, b,
					(branch_s *)ahead, (branch_s *)bhead);
			break;
	default:	eprintf("compare_nodes niether leaf nor branch");
			rc = qERR_BAD_TREE;
			break;
	}
	return rc;
}

int compare_trees (tree_s *a, tree_s *b)
{
	return compare_nodes(a, b, root(a), root(b));
}

rec_s *binary_search_leaf (u32 key, rec_s *keys, snint n)
{
	snint	x, left, right;	/* left and right must be
				 * signed intergers */
	rec_s	*r;
FN;
	left = 0;
	right = n - 1;
	while (left <= right) {
		x = (left + right) / 2;
		r = &keys[x];
		if (key == r->r_key) return r;
		if (key < r->r_key) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return NULL;
}

rec_s *binary_search_leaf_or_next (u32 key, rec_s *keys, snint n)
{
	snint	x, left, right;	/* left and right must be
				 * signed intergers */
	rec_s	*r;
FN;
	left = 0;
	right = n - 1;
	while (left <= right) {
		x = (left + right) / 2;
		r = &keys[x];
		if (key == r->r_key) return r;
		if (key < r->r_key) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return &keys[left];
}

snint binary_search_branch (u32 key, key_s *keys, snint n)
{
	snint	x, left, right;	/* left and right must be
				 * signed intergers */
	key_s	*k;
FN;
	left = 0;
	right = n - 1;
	while (left <= right) {
		x = (left + right) / 2;
		k = &keys[x];
		if (key == k->k_key) return x;
		if (key < k->k_key) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return right;
}

static inline bool found (leaf_s *leaf, rec_s *r, u32 key)
{
	return r && (r < &leaf->l_rec[leaf->l_num])
		 && (r->r_key == key);
}

static inline rec_s *find_rec (leaf_s *leaf, u32 key)
{
	rec_s	*rec;
FN;
	rec = binary_search_leaf_or_next(key, leaf->l_rec, leaf->l_num);
	if (found(leaf, rec, key)) {
		return rec;
	} else {
		return NULL;
	}
}

static inline rec_s *leaf_search (void *blk, u32 key)
{
	leaf_s	*leaf = blk;
FN;
	return binary_search_leaf_or_next(key, leaf->l_rec, leaf->l_num);
}

void copy_recs (leaf_s *dst, leaf_s *src, unint start)
{
	rec_s	*s;
	rec_s	*d;
	unint	i;
FN;
	for (i = start; i < src->l_num; i++) {
		s = &src->l_rec[i];
		d = &dst->l_rec[dst->l_num++];
		dst->l_total -= s->r_size + sizeof(rec_s);
		dst->l_end  -= s->r_size;
		d->r_start   = dst->l_end;
		d->r_key     = s->r_key;
		d->r_size    = s->r_size;
		memmove((char *)dst + d->r_start,
			(char *)src + s->r_start,
			s->r_size);
	}
}

static void compact (leaf_s *leaf)
{
	static char	block[PAGE_SIZE];
	leaf_s		*p;
FN;
	// Need a spin lock here
	p = (leaf_s *)block;
	memset(p, 0, PAGE_SIZE);

	p->h_node = leaf->h_node;
	p->h_magic  = leaf->h_magic;
	p->l_end    = PAGE_SIZE;
	p->l_total  = MAX_FREE;

	copy_recs(p, leaf, 0);
	memmove(leaf, p, PAGE_SIZE);
	assert(leaf->l_total == free_space(leaf));
}

static inline int branch_is_full (branch_s *branch)
{
	return branch->br_num == KEYS_PER_BRANCH;
}

static int leaf_is_full (leaf_s *leaf, unint size)
{
	snint	total = size + sizeof(rec_s);

	if (total <= free_space(leaf)) {
		return FALSE;
	}
	if (total > leaf->l_total) {
		return TRUE;
	}
	compact(leaf);
	return FALSE;
}

void *grow_tree (tree_s *tree, unint size)
{
	void		*node;
	branch_s	*branch;
	int		rc;
FN;
	node = root(tree);
	if (node) {
		switch (magic(node)) {
		case LEAF:
			if (!leaf_is_full(node, size)) {
				return node;
			}
			break;
		case BRANCH:
			if (!branch_is_full(node)) {
				return node;
			}
			break;
		default:
			eprintf("Bad node");
			break;
		}
		branch = new_branch(tree);
		branch->br_first = node;
		node = branch;
	} else {
		node = new_leaf(tree);
	}
	rc = change_root(tree, node);
	if (rc) {
		eprintf("Couldn't grow tree");
		return NULL;
	}
	return node;
}

void insert_sibling (branch_s *parent, void *sibling, s64 k, u32 key)
{
	snint		slot;
FN;
	/* make a hole -- only if not at end */
	slot = k + 1;
	if (slot < parent->br_num) {
		memmove( &parent->br_key[slot+1],
			 &parent->br_key[slot],
			 sizeof(key_s) * (parent->br_num - slot));
	}
	parent->br_key[slot].k_key   = key;
	parent->br_key[slot].k_node = sibling;
	++parent->br_num;
}

bool split_leaf (tree_s *tree, branch_s *parent, leaf_s *child, s64 k, unint size)
{
	leaf_s	*sibling;
	snint	upper;
	u32	upper_key;

	if (!leaf_is_full(child, size)) {
		return FALSE;
	}
FN;
	sibling = new_leaf(tree);

	upper = (child->l_num + 1) / 2;
	upper_key = child->l_rec[upper].r_key;

	copy_recs(sibling, child, upper);

	child->l_num = upper;
	child->l_total += (PAGE_SIZE - sizeof(leaf_s)) - sibling->l_total;

	insert_sibling(parent, sibling, k, upper_key);
	return TRUE;
}

bool split_branch (tree_s *tree, branch_s *parent, branch_s *child, s64 k)
{
	branch_s	*sibling;
	snint		upper;
	snint		midpoint;
	u32		upper_key;

	if (!branch_is_full(child)) return FALSE;
FN;
	sibling = new_branch(tree);

	midpoint = child->br_num / 2;
	upper = midpoint + 1;

	sibling->br_first = child->br_key[midpoint].k_node;
	upper_key         = child->br_key[midpoint].k_key;

	memmove(sibling->br_key, &child->br_key[upper],
		sizeof(key_s) * (child->br_num - upper));
	sibling->br_num = child->br_num - upper;

	child->br_num = midpoint;

	insert_sibling(parent, sibling, k, upper_key);
	return TRUE;
}

static bool split_node (
	tree_s		*tree,
	branch_s	*parent,
	void		*child,
	s64		k,
	unint		size)
{
	switch (magic(child)) {
	case LEAF:	return split_leaf(tree, parent, child, k, size);
	case BRANCH:	return split_branch(tree, parent, child, k);
	default:	return FALSE;
	}
}

int insert_leaf (
	tree_s	*tree,
	leaf_s	*leaf,
	u32	key,
	void	*rec,
	unint	size)
{
	rec_s	*r;
	snint	total = size + sizeof(rec_s);
FN;
	r = leaf_search(leaf, key);
	if (found(leaf, r, key)) {
		return qERR_DUP;
	}

	memmove(r + 1, r, (char *)&leaf->l_rec[leaf->l_num] - (char *)r);
	++leaf->l_num;
	leaf->l_total -= total;
	leaf->l_end   -= size;
	r->r_start     = leaf->l_end;
	r->r_size      = size;
	r->r_key       = key;
	pack_rec(tree, (u8 *)leaf + r->r_start, rec, size);

	return 0;
}

static int insert_node (
	tree_s	*tree,
	void	*node,
	u32	key,
	void	*rec,
	unint	size);

static int insert_branch (
	tree_s		*tree,
	branch_s	*parent,
	u32		key,
	void		*rec,
	unint		size)
{
	void		*child;
	s64		k;	/* Critical that this be signed */
FN;
	for (;;) {
		do {
			k = binary_search_branch(key, parent->br_key,
							parent->br_num);
			child = parent->br_key[k].k_node;
			if (!child) {
				return qERR_NOT_FOUND;
			}
		} while (split_node(tree, parent, child, k, size));
		if (magic(child) != BRANCH) {
			return insert_node(tree, child, key, rec, size);
		}
		parent = child;
	}
}

static int insert_node (tree_s *tree, void *node, u32 key, void *rec, unint size)
{
FN;
	switch (magic(node)) {
	case LEAF:	return insert_leaf(tree, node, key, rec, size);
	case BRANCH:	return insert_branch(tree, node, key, rec, size);
	default:	return qERR_BAD_BLOCK;
	}
}

int insert (tree_s *tree, u32 key, void *rec, unint size)
{
	void	*node;
FN;
	node = grow_tree(tree, size);
	return insert_node(tree, node, key, rec, size);
}

int delete_leaf (tree_s *tree, leaf_s *leaf, u32 key)
{
	rec_s	*r;
FN;
	r = find_rec(leaf, key);
	if (r) {
		leaf->l_total += sizeof(rec_s) + r->r_size;
		memmove(r, r + 1,
			(char *)&leaf->l_rec[--leaf->l_num] - (char *)r);
		return 0;
	}
	eprintf("Key not found %llx\n", key);
	return qERR_NOT_FOUND;
}

static int join_leaf (tree_s *tree, branch_s *parent, leaf_s *child, snint k)
{
	leaf_s		*sibling;

	sibling = parent->br_key[k+1].k_node;
	if (!sibling) return qERR_NOT_FOUND;

	if (child->l_total + sibling->l_total > MAX_FREE) {
		compact(child);
		copy_recs(child, sibling, 0);
		memmove( &parent->br_key[k+1], &parent->br_key[k+2],
			sizeof(parent->br_key[0]) * (parent->br_num - (k+2)));
		--parent->br_num;
	}
	return 0;
}

static int join_branch (tree_s *tree, branch_s *parent, branch_s *child, snint k)
{
	branch_s	*sibling;

	sibling = parent->br_key[k+1].k_node;
	if (!sibling) return qERR_NOT_FOUND;

	if (child->br_num+sibling->br_num < KEYS_PER_BRANCH-1) {
		child->br_key[child->br_num].k_key = parent->br_key[k+1].k_key;
		child->br_key[child->br_num].k_node = sibling->br_first;
		++child->br_num;
		memmove( &child->br_key[child->br_num], sibling->br_key,
			sibling->br_num * sizeof(sibling->br_key[0]));
		child->br_num += sibling->br_num;

		memmove( &parent->br_key[k+1], &parent->br_key[k+2],
			sizeof(parent->br_key[0]) * (parent->br_num - (k+2)));
		--parent->br_num;
	}
	return 0;
}

//XXX: Might be better to join to the left rather than the right
static int join_node (tree_s *tree, branch_s *parent, snint k)
{
	void	*child;
	int	rc;
FN;
	if (k == parent->br_num - 1) {
		/* no sibling to the right */
		return 0;
	}
	child = parent->br_key[k].k_node;
	if (!child) return qERR_NOT_FOUND;

	switch (magic(child)) {
	case LEAF:	rc = join_leaf(tree, parent, child, k);
			break;
	case BRANCH:	rc = join_branch(tree, parent, child, k);
			break;
	default:	rc = qERR_BAD_BLOCK;
			break;
	}
	return rc;
}

int delete (tree_s *tree, u32 key)
{
	void		*child;
	branch_s	*parent;
	snint		k;
	int		rc;
FN;
	child = root(tree);
	for (;;) {
		if (!child) return qERR_NOT_FOUND;

		if (magic(child) == LEAF) {
			break;
		}
		parent = child;
		k = binary_search_branch(key, parent->br_key, parent->br_num);
		join_node(tree, parent, k);
		child = parent->br_key[k].k_node;
	}
	rc = delete_leaf(tree, child, key);
	return rc;
}

int find_or_next_leaf (
	leaf_s	*leaf,
	u32	key,
	u32	*found_or_next_key,
	char	*name)
{
	rec_s	*rec;

	rec = binary_search_leaf_or_next(key, leaf->l_rec, leaf->l_num);
	if (rec == &leaf->l_rec[leaf->l_num]) {
		return qERR_TRY_NEXT;
	}
	assert(key <= rec->r_key);
	*found_or_next_key = rec->r_key;
	if (name) {
		memcpy(name, (u8 *)leaf + rec->r_start, rec->r_size);
	}
	return 0;
}

int search_leaf (
	tree_s		*tree,
	leaf_s		*leaf,
	u32		key,
	search_f	sf,
	void		*data)
{
	rec_s	*rec;
	int	rc;

	rec = binary_search_leaf_or_next(key, leaf->l_rec, leaf->l_num);
	assert(rec);
	for (;;) {
		if (rec == &leaf->l_rec[leaf->l_num]) {
			return qERR_TRY_NEXT;
		}
		rc = sf(data, rec->r_key,
			(char *)leaf + rec->r_start, rec->r_size);
		if (rc != qERR_TRY_NEXT) {
			return rc;
		}
		++rec;
	}
}

int search_node (tree_s *tree, void *node, u32 key, search_f sf, void *data);

int search_branch (
	tree_s		*tree,
	branch_s	*parent,
	u32		key,
	search_f	sf,
	void		*data)
{
	head_s		*child;
	snint		k;
	int		rc;

	k = binary_search_branch(key, parent->br_key, parent->br_num);
	do {
		child = parent->br_key[k].k_node;
		if (!child) return qERR_NOT_FOUND;

		rc = search_node(tree, child, key, sf, data);
		if (rc != qERR_TRY_NEXT) {
			return rc;
		}
		++k;
	} while (k != parent->br_num);
	return rc;
}

int search_node (tree_s *tree, void *node, u32 key, search_f sf, void *data)
{
FN;
	switch (magic(node)) {
	case LEAF:	return search_leaf(tree, node, key, sf, data);
	case BRANCH:	return search_branch(tree, node, key, sf, data);
	default:	return qERR_BAD_BLOCK;
	}
}

int search (
	tree_s		*tree,
	u32		key,
	search_f	sf,
	void		*data)
{
	void	*node;
	int	rc;
FN;
	node = root(tree);
	if (!node) return qERR_NOT_FOUND;

	rc = search_node(tree, node, key, sf, data);

	return rc;
}
