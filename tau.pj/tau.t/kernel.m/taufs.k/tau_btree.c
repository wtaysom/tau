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
#include <tau_err.h>
#include <tau_blk.h>
#include <tau_fs.h>

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
	__u64	k_key;
	__u64	k_block;
} key_s;

typedef struct rec_s {
	__s16	r_start;
	__s16	r_size;
	__u16	r_reserved[2];
	__u64	r_key;
} rec_s;

/* b0 k1 b1 k2 b2 k3 b3 */
typedef struct branch_s {
	HEAD_S;
	__s16	br_num;
	__u16	br_reserved[3];
	__u64	br_first;
	key_s	br_key[0];
} branch_s;

typedef struct leaf_s {
	HEAD_S;
	__s16	l_num;
	__s16	l_end;
	__s16	l_total;	// change to free later
	__u16	l_reserved[1];
	// Another way to make end work would be to use
	// &l_rec[l_num] as the index point of l_end
	// That is what I did in insert!!!
	rec_s	l_rec[0];
} leaf_s;

enum {
	KEYS_PER_BRANCH = (BLK_SIZE - sizeof(branch_s)) / sizeof(key_s),
	MAX_FREE = BLK_SIZE - sizeof(leaf_s)
};

static inline struct address_space *tree_inode (tree_s *tree)
{
	return tree->t_inode->i_mapping;
}

static void dump_node (
	tree_s	*tree,
	u64	blknum,
	unint	indent);

static void error (char *msg)
{
	eprintk("ERROR: %s\n", msg);
}

static char *indent (unint i)
{
	static char blanks[] = "                          ";

	i *= 4;

	if (i >= sizeof(blanks)) return blanks;
	return &blanks[sizeof(blanks) - 1 - i];
}

#if 0
static void pr_leaf (leaf_s *leaf)
{
	dprintk("LEAF blknum=%llx num=%x end=%x total=%x\n",
		leaf->h_blknum, leaf->l_num, leaf->l_end, leaf->l_total);
}
#endif

static inline u32 magic (void *data)
{
	head_s	*h = data;

	return h->h_magic;
}

static void *alloc_block (tree_s *tree)
{
	struct inode	*inode = tree->t_inode;
	void	*data;
	u64	blknum;
FN;
	blknum = tau_alloc_block(inode->i_sb, 1);
	if (!blknum) return NULL;

	data = tau_bnew(inode->i_mapping, blknum);
	return data;
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

static void dump_leaf (
	tree_s	*tree,
	leaf_s	*leaf,
	unint	depth)
{
	rec_s	*end = &leaf->l_rec[leaf->l_num];
	rec_s	*r;
	char	*c;
	unint	i;

	if (!leaf) return;

	dprintk("%sleaf: num keys = %d end = %d total = %d current = %ld\n",
			indent(depth),
			leaf->l_num, leaf->l_end, leaf->l_total,
			free_space(leaf));
	for (r = leaf->l_rec, i = 0; r < end; r++, i++) {
		c = (char *)leaf + r->r_start;
		dprintk("%s    %4lu. %16llx %4d:%4d %s\n",
			indent(depth),
			i, r->r_key, r->r_start, r->r_size,
			dump_rec(tree, r->r_key, c, r->r_size));

	}
}

static void dump_branch (
	tree_s		*tree,
	branch_s	*branch,
	unint		depth)
{
	key_s	*k;
	unint	i;

	if (!branch) return;

	dprintk("%sbranch: num keys = %d\n",
		indent(depth), branch->br_num);
	dprintk("%sfirst %llx\n", indent(depth), branch->br_first);
	dump_node(tree, branch->br_first, depth+1);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		dprintk("%s%4lu. %16llx: %llx\n",
			indent(depth),
			i, k->k_key, k->k_block);
		dump_node(tree, k->k_block, depth+1);
	}
}

static void dump_node (
	tree_s	*tree,
	u64	blknum,
	unint	depth)
{
	head_s	*h;

	h = tau_bget(tree_inode(tree), blknum);
	if (!h) return;

	dprintk("blknum=%llx\n", tau_bnum(h));
	switch (h->h_magic) {
	case LEAF:	dump_leaf(tree, (leaf_s *)h, depth);
			break;
	case BRANCH:	dump_branch(tree, (branch_s *)h, depth);
			break;
	default:	tau_prmem("Unknown block magic", h, BLK_SIZE);
			break;
	}
	tau_bput(h);
}

void tau_dump_tree (tree_s *tree)
{
	dprintk("Tree------------------------------------------------\n");
	dump_node(tree, root(tree), 0);
}

#if 0
static void verify_leaf (tree_s *tree, leaf_s *leaf, char *where)
{
	snint	sum;
	unint	i;

	if (leaf->h_magic != LEAF) {
		eprintk("ERROR:%s leaf=%p magic=%x\n",
			where, leaf, leaf->h_magic);
		BUG();
	}
	if (leaf->l_total < free_space(leaf)) {
		eprintk("ERROR:%s allocation space bigger than total\n",
			where);
		dump_leaf(tree, leaf, 0);
		BUG();
	}
	if (free_space(leaf) < 0) {
		eprintk("ERROR:%s leaf overflow\n", where);
		dump_leaf(tree, leaf, 0);
		BUG();
	}
	sum = sizeof(leaf_s) + (leaf->l_num * sizeof(rec_s));
	for (i = 0; i < leaf->l_num; i++) {
		sum += leaf->l_rec[i].r_size;
	}
	if (BLK_SIZE - sum != leaf->l_total) {
		eprintk("ERROR:%s totals %ld != %d\n", where,
			BLK_SIZE - sum, leaf->l_total);
		dump_leaf(tree, leaf, 0);
		BUG();
	}
	for (i = 1; i < leaf->l_num; i++) {
		if (leaf->l_rec[i-1].r_key >= leaf->l_rec[i].r_key) {
			eprintk("ERROR:%s keys out of order %llx >= %llx\n",
				where,
				leaf->l_rec[i-1].r_key, leaf->l_rec[i].r_key);
			dump_leaf(tree, leaf, 0);
			BUG();
		}
	}
			
}
#endif

static leaf_s *new_leaf (tree_s *tree)
{
	leaf_s	*leaf;
FN;
	leaf = alloc_block(tree);
	leaf->h_blknum = tau_bnum(leaf);
	leaf->h_magic  = LEAF;
	leaf->l_end    = BLK_SIZE;
	leaf->l_total  = MAX_FREE;
	return leaf;
}

static branch_s *new_branch (tree_s *tree)
{
	branch_s	*branch;
FN;
	branch = alloc_block(tree);
	branch->h_blknum = tau_bnum(branch);
	branch->h_magic  = BRANCH;
	return branch;
}

static void stat_node (
	struct address_space	*mapping,
	u64		blknum,
	btree_stat_s	*bs,
	unint		height);

static void stat_leaf (leaf_s *leaf, btree_stat_s *bs)
{
	++bs->st_num_leaves;
	bs->st_leaf_free_space += leaf->l_total;
	bs->st_leaf_recs       += leaf->l_num;
}

static void stat_branch (
	struct address_space	*mapping,
	branch_s		*branch,
	btree_stat_s		*bs,
	unint			height)
{
	key_s	*k;
	unint	i;

	++bs->st_num_branches;
	bs->st_branch_recs += branch->br_num;

	stat_node(mapping, branch->br_first, bs, height);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		stat_node(mapping, k->k_block, bs, height);
	}
}

static void stat_node (
	struct address_space	*mapping,
	u64			blknum,
	btree_stat_s		*bs,
	unint			height)
{
	head_s	*h;

	h = tau_bget(mapping, blknum);
	if (!h) return;

	switch (h->h_magic) {
	case LEAF:	stat_leaf((leaf_s *)h, bs);
			break;
	case BRANCH:	stat_branch(mapping, (branch_s *)h, bs, height + 1);
			break;
	default:	eprintk("Unknown block magic %x\n", h->h_magic);
			break;
	}
	if (height > bs->st_height) {
		bs->st_height = height;
	}
	tau_bput(h);
}

void tau_stat_tree (tree_s *tree, btree_stat_s *bs)
{
	zero(*bs);
	bs->st_blk_size = BLK_SIZE;
	stat_node(tree_inode(tree), root(tree), bs, 1);
}

static int compare_nodes (tree_s *a, tree_s *b, u64 a_blk, u64 b_blk);

static int compare_rec (rec_s *a, rec_s *b)
{
	if (a->r_key != b->r_key) {
		eprintk("rec key doesn't match %llx!=%llx\n",
			a->r_key, b->r_key);
		return qERR_BAD_TREE;
	}
	if (a->r_start != b->r_start) {
		eprintk("rec start doesn't match %x!=%x\n",
			a->r_start, b->r_start);
		return qERR_BAD_TREE;
	}
	if (a->r_size != b->r_size) {
		eprintk("rec size doesn't match %x!=%x\n",
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
		eprintk("leaf num doesn't match %x!=%x\n",
			aleaf->l_num, bleaf->l_num);
		return qERR_BAD_TREE;
	}
	if (aleaf->l_end != bleaf->l_end) {
		eprintk("leaf end doesn't match %x!=%x\n",
			aleaf->l_end, bleaf->l_end);
		return qERR_BAD_TREE;
	}
	if (aleaf->l_total != bleaf->l_total) {
		eprintk("leaf total doesn't match %x!=%x\n",
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
		eprintk("branch num doesn't match %x!=%x\n",
			abranch->br_num, bbranch->br_num);
		return qERR_BAD_TREE;
	}
	rc = compare_nodes(a, b, abranch->br_first, bbranch->br_first);
	if (rc) return rc;
	for (i = 0; i < abranch->br_num; i++) {
		akey = &abranch->br_key[i];
		bkey = &bbranch->br_key[i];
		if (akey->k_key != bkey->k_key) {
			eprintk("branch key %lu doesn't match %llx!=%llx\n",
				i, akey->k_key, bkey->k_key);
			return qERR_BAD_TREE;
		}
		if (akey->k_block != bkey->k_block) {
			eprintk("key block %lu doesn't match %llx!=%llx\n",
				i, akey->k_block, bkey->k_block);
			return qERR_BAD_TREE;
		}
		rc = compare_nodes(a, b, akey->k_block, bkey->k_block);
		if (rc) return rc;
	}
	return 0;	
}

static int compare_nodes (tree_s *a_tree, tree_s *b_tree, u64 a_blk, u64 b_blk)
{
	head_s	*a;
	head_s	*b;
	int	rc;

	a = tau_bget(tree_inode(a_tree), a_blk);
	if (!a) {
		eprintk("compare_nodes couldn't get 'a' blk %llx", a_blk);
		return qERR_BAD_TREE;
	}
	b = tau_bget(tree_inode(b_tree), b_blk);
	if (!b) {
		eprintk("compare_nodes couldn't get 'b' blk %llx", b_blk);
		return qERR_BAD_TREE;
	}
	if (memcmp(a, b, BLK_SIZE) != 0) {
		eprintk("a and b are not binary identical\n");
	}

	if (a->h_magic != b->h_magic) {
		eprintk("Magic doesn't match %x!=%x\n",
			a->h_magic, b->h_magic);
		tau_bput(a);
		tau_bput(b);
		return qERR_BAD_TREE;
	}
	if (a->h_blknum != b->h_blknum) {
		eprintk("Blknum doesn't match %llx!=%llx\n",
			a->h_blknum, b->h_blknum);
		tau_bput(a);
		tau_bput(b);
		return qERR_BAD_TREE;
	}
	switch (a->h_magic) {
	case LEAF:	rc = compare_leaves(a_tree, b_tree,
					(leaf_s *)a, (leaf_s *)b);
			break;
	case BRANCH:	rc = compare_branches(a_tree, b_tree,
					(branch_s *)a, (branch_s *)b);
			break;
	default:	eprintk("compare_nodes niether leaf nor branch");
			rc = qERR_BAD_TREE;
			break;
	}
	tau_bput(a);
	tau_bput(b);
	return rc;
}

int tau_compare_trees (tree_s *a, tree_s *b)
{
	return compare_nodes(a, b, root(a), root(b));
}

#if 0
static rec_s *binary_search_leaf (u64 key, rec_s *keys, snint n)
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
#endif

static rec_s *binary_search_leaf_or_next (u64 key, rec_s *keys, snint n)
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

static snint binary_search_branch (u64 key, key_s *keys, snint n)
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

static inline bool found (leaf_s *leaf, rec_s *r, u64 key)
{
	return r && (r < &leaf->l_rec[leaf->l_num])
		 && (r->r_key == key);
}

static inline rec_s *find_rec (leaf_s *leaf, u64 key)
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

static inline rec_s *leaf_search (void *blk, u64 key)
{
	leaf_s	*leaf = blk;
FN;
	return binary_search_leaf_or_next(key, leaf->l_rec, leaf->l_num);
}

static void copy_recs (leaf_s *dst, leaf_s *src, unint start)
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
	static char	block[BLK_SIZE];
	leaf_s		*p;
FN;
	// Need a spin lock here
	p = (leaf_s *)block;
	memset(p, 0, BLK_SIZE);

	p->h_blknum = leaf->h_blknum;
	p->h_magic  = leaf->h_magic;
	p->l_end    = BLK_SIZE;
	p->l_total  = MAX_FREE;

	copy_recs(p, leaf, 0);
	memmove(leaf, p, BLK_SIZE);
	assert(leaf->l_total == free_space(leaf));
	tau_blog(leaf);
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

static void *grow_tree (tree_s *tree, unint size)
{
	head_s		*head;
	branch_s	*branch;
	int		rc;
FN;
	if (root(tree)) {
		head = tau_bget(tree_inode(tree), root(tree));
		if (!head) return NULL;

		switch (head->h_magic) {
		case LEAF:
			if (!leaf_is_full((leaf_s *)head, size)) {
				return head;
			}
			break;
		case BRANCH:
			if (!branch_is_full((branch_s *)head)) {
				return head;
			}
			break;
		default:
			tau_bput(head);
			eprintk("Bad block");
			BUG();
			break;
		}
		branch = new_branch(tree);
		branch->br_first = tau_bnum(head);
		tau_bput(head);
		head = (head_s *)branch;
	} else {
		head = (head_s *)new_leaf(tree);
	}
	rc = change_root(tree, head);
	if (rc) {
		error("Couldn't grow tree");
		return NULL;
	}
	return head;
}

static void insert_sibling (branch_s *parent, void *sibling, s64 k, u64 key)
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
	parent->br_key[slot].k_block = tau_bnum(sibling);
	++parent->br_num;
	tau_blog(parent);
}

static bool split_leaf (tree_s *tree, leaf_s *child, branch_s *parent, s64 k, unint size)
{
	leaf_s	*sibling;
	snint	upper;
	u64	upper_key;

	if (!leaf_is_full(child, size)) {
		return FALSE;
	}
FN;
	sibling = new_leaf(tree);

	upper = (child->l_num + 1) / 2;
	upper_key = child->l_rec[upper].r_key;

	copy_recs(sibling, child, upper);

	child->l_num = upper;
	child->l_total += (BLK_SIZE - sizeof(leaf_s)) - sibling->l_total;
	tau_blog(child);
	tau_bput(child);

	insert_sibling(parent, sibling, k, upper_key);
	tau_bput(sibling);
	return TRUE;
}

static bool split_branch (tree_s *tree, branch_s *child, branch_s *parent, s64 k)
{
	branch_s	*sibling;
	snint		upper;
	snint		midpoint;
	u64		upper_key;

	if (!branch_is_full(child)) return FALSE;
FN;
	sibling = new_branch(tree);

	midpoint = child->br_num / 2;
	upper = midpoint + 1;

	sibling->br_first = child->br_key[midpoint].k_block;
	upper_key         = child->br_key[midpoint].k_key;

	memmove(sibling->br_key, &child->br_key[upper],
		sizeof(key_s) * (child->br_num - upper));
	sibling->br_num = child->br_num - upper;

	child->br_num = midpoint;
	tau_blog(child);
	tau_bput(child);

	insert_sibling(parent, sibling, k, upper_key);
	tau_bput(sibling);
	return TRUE;
}

static bool split_node (
	tree_s		*tree,
	void		*child,
	branch_s	*parent,
	s64		k,
	unint		size)
{
	switch (magic(child)) {
	case LEAF:	return split_leaf(tree, child, parent, k, size);
	case BRANCH:	return split_branch(tree, child, parent, k);
	default:	return FALSE;
	}
}

static int insert_leaf (
	tree_s	*tree,
	leaf_s	*leaf,
	u64	key,
	void	*rec,
	unint	size)
{
	rec_s	*r;
	snint	total = size + sizeof(rec_s);
FN;
	r = leaf_search(leaf, key);
	if (found(leaf, r, key)) {
		tau_bput(leaf);
		return qERR_DUP;
	}
	memmove(r + 1, r, (char *)&leaf->l_rec[leaf->l_num] - (char *)r);
	++leaf->l_num;
	leaf->l_total -= total;
	leaf->l_end -= size;
	r->r_start = leaf->l_end;
	r->r_size   = size;
	r->r_key   = key;
	pack_rec(tree, (u8 *)leaf + r->r_start, rec, size);

	tau_blog(leaf);
	tau_bput(leaf);
	return 0;
}

static int insert_node (
	tree_s	*tree,
	void	*node,
	u64	key,
	void	*rec,
	unint	size);

static int insert_branch (
	tree_s		*tree,
	branch_s	*parent,
	u64		key,
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
			child = tau_bget(tree_inode(tree),
					parent->br_key[k].k_block);
			if (!child) {
				tau_bput(parent);
				return qERR_NOT_FOUND;
			}
		} while (split_node(tree, child, parent, k, size));
		tau_bput(parent);
		if (magic(child) != BRANCH) {
			return insert_node(tree, child, key, rec, size);
		}
		parent = child;
	}
}

static int insert_node (
	tree_s	*tree,
	void	*node,
	u64	key,
	void	*rec,
	unint	size)
{
FN;
	switch (magic(node)) {
	case LEAF:	return insert_leaf(tree, node, key, rec, size);
	case BRANCH:	return insert_branch(tree, node, key, rec, size);
	default:	return qERR_BAD_BLOCK;
	}
}

int tau_insert (tree_s *tree, u64 key, void *rec, unint size)
{
	void	*child;
FN;
	child = grow_tree(tree, size);
	return insert_node(tree, child, key, rec, size);
}

static int delete_leaf (tree_s *tree, leaf_s *leaf, u64 key)
{
	rec_s	*r;
FN;
	r = find_rec(leaf, key);
	if (r) {
		leaf->l_total += sizeof(rec_s) + r->r_size;
		memmove(r, r + 1,
			(char *)&leaf->l_rec[--leaf->l_num] - (char *)r);
		tau_blog(leaf);
		return 0;
	}
	eprintk("Key not found %llx\n", key);
	return qERR_NOT_FOUND;
}

static int join_leaf (tree_s *tree, branch_s *parent, leaf_s *child, snint k)
{
	leaf_s		*sibling;

	sibling = tau_bget(tree_inode(tree), parent->br_key[k+1].k_block);
	if (!sibling) return qERR_NOT_FOUND;

	if (child->l_total + sibling->l_total > MAX_FREE) {
HERE;
		compact(child);
		copy_recs(child, sibling, 0);
		tau_blog(child);
		memmove( &parent->br_key[k+1], &parent->br_key[k+2],
			sizeof(parent->br_key[0]) * (parent->br_num - (k+2)));
		--parent->br_num;
		tau_blog(parent);
		tau_blog(sibling);
	}
	tau_bput(sibling);	//XXX: Should free sibling
	return 0;
}

static int join_branch (tree_s *tree, branch_s *parent, branch_s *child, snint k)
{
	branch_s	*sibling;

	sibling = tau_bget(tree_inode(tree), parent->br_key[k+1].k_block);
	if (!sibling) {
		return qERR_NOT_FOUND;
	}
	if (child->br_num+sibling->br_num < KEYS_PER_BRANCH-1) {
HERE;
		child->br_key[child->br_num].k_key = parent->br_key[k+1].k_key;
		child->br_key[child->br_num].k_block = sibling->br_first;
		++child->br_num;
		memmove( &child->br_key[child->br_num], sibling->br_key,
			sibling->br_num * sizeof(sibling->br_key[0]));
		child->br_num += sibling->br_num;
		tau_blog(child);

		memmove( &parent->br_key[k+1], &parent->br_key[k+2],
			sizeof(parent->br_key[0]) * (parent->br_num - (k+2)));
		--parent->br_num;
		tau_blog(sibling);
		tau_blog(parent);
	}
	tau_bput(sibling);	//XXX: Sibling should be freed.
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
	child = tau_bget(tree_inode(tree), parent->br_key[k].k_block);
	if (!child) return qERR_NOT_FOUND;

	switch (magic(child)) {
	case LEAF:	rc = join_leaf(tree, parent, child, k);
			break;
	case BRANCH:	rc = join_branch(tree, parent, child, k);
			break;
	default:	rc = qERR_BAD_BLOCK;
			break;
	}
	tau_bput(child);
	return rc;
}

int tau_delete (tree_s *tree, u64 key)
{
	void		*child;
	branch_s	*parent;
	snint		k;
	int		rc;
FN;
	child = tau_bget(tree_inode(tree), root(tree));
	for (;;) {
		if (!child) return qERR_NOT_FOUND;

		if (magic(child) == LEAF) {
			break;
		}
		parent = child;
		k = binary_search_branch(key, parent->br_key, parent->br_num);
		join_node(tree, parent, k);
		child = tau_bget(tree_inode(tree), parent->br_key[k].k_block);
		tau_bput(parent);
	}
	rc = delete_leaf(tree, child, key);
	tau_bput(child);
	return rc;
}

#if 0
static int find_or_next_leaf (
	leaf_s	*leaf,
	u64	key,
	u64	*found_or_next_key,
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
		memcpy(name, (char *)leaf + rec->r_start,
			rec->r_size);
	}
	return 0;
}
#endif

static int search_leaf (
	tree_s		*tree,
	leaf_s		*leaf,
	u64		key,
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

static int search_node (tree_s *tree, void *node, u64 key, search_f sf, void *data);

static int search_branch (
	tree_s		*tree,
	branch_s	*parent,
	u64		key,
	search_f	sf,
	void		*data)
{
	void		*child;
	snint		k;
	int		rc;

	k = binary_search_branch(key, parent->br_key, parent->br_num);
	do {
		child = tau_bget(tree_inode(tree), parent->br_key[k].k_block);
		if (!child) return qERR_NOT_FOUND;

		rc = search_node(tree, child, key, sf, data);
		tau_bput(child);
		if (rc != qERR_TRY_NEXT) {
			return rc;
		}
		++k;
	} while (k != parent->br_num);
	return rc;
}

static int search_node (tree_s *tree, void *node, u64 key, search_f sf, void *data)
{
FN;
	switch (magic(node)) {
	case LEAF:	return search_leaf(tree, node, key, sf, data);
	case BRANCH:	return search_branch(tree, node, key, sf, data);
	default:	return qERR_BAD_BLOCK;
	}
}

int tau_search (
	tree_s		*tree,
	u64		key,
	search_f	sf,
	void		*data)
{
	void		*parent;
	int		rc;
FN;
	parent = tau_bget(tree_inode(tree), root(tree));
	if (!parent) return qERR_NOT_FOUND;

	rc = search_node(tree, parent, key, sf, data);
	tau_bput(parent);

	return rc;
}
