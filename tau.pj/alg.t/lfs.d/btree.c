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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <tauerr.h>

#include "blk.h"
#include "btree.h"

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

typedef enum {	LEAF = 0x1eaf,
		BRANCH = 0xbac4 } block_t;

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
	__u16	br_reserved[3];	// Should make sure is 0
	__u64	br_first;
	key_s	br_key[0];
} branch_s;

typedef struct leaf_s {
	HEAD_S;
	__s16	l_num;
	__s16	l_end;
	__s16	l_total;	// change to free later
	__u16	l_reserved[1];	// Should make sure is 0
	// Another way to make end work would be to use
	// &l_rec[l_num] as the index point of l_end
	// That is what I did in insert!!!
	rec_s	l_rec[0];
} leaf_s;

enum {
	KEYS_PER_BRANCH = (BLK_SIZE - sizeof(branch_s)) / sizeof(key_s),
	MAX_FREE = BLK_SIZE - sizeof(leaf_s)
};

void dump_node (
	tree_s	*tree,
	u64	blknum,
	unint	indent);

void error (char *msg)
{
	printf("ERROR: %s\n", msg);
}

void pr_indent (unsigned indent)
{
	while (indent--) {
		printf("    ");
	}
}

void pr_leaf (buf_s *buf)
{
	leaf_s	*leaf = buf->b_data;

	printf("LEAF blknum=%llx num=%x end=%x total=%x\n",
		leaf->h_blknum, leaf->l_num, leaf->l_end, leaf->l_total);
}

static inline unint magic (buf_s *buf)
{
	head_s	*h;

	h = buf->b_data;
	return h->h_magic;
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

	pr_indent(depth);
	printf("leaf: level = %d num keys = %d end = %d total = %d current = %ld\n",
			leaf->h_level, leaf->l_num, leaf->l_end, leaf->l_total,
			free_space(leaf));
	fflush(stdout);
	for (r = leaf->l_rec, i = 0; r < end; r++, i++) {
		c = (char *)leaf + r->r_start;
		pr_indent(depth);
		printf("\t%4ld. %16llx %4d:%4d ",
			i, r->r_key, r->r_start, r->r_size);

		dump_rec(tree, r->r_key, c, r->r_size);

		printf("\n");
		fflush(stdout);
	}
	fflush(stdout);
}

void dump_branch (
	tree_s		*tree,
	branch_s	*branch,
	unint		depth)
{
	key_s	*k;
	unint	i;

	if (!branch) return;

	pr_indent(depth);
	printf("branch: level = %d num keys = %d\n",
		branch->h_level, branch->br_num);
	pr_indent(depth);
	printf("first %llx\n", branch->br_first);
	dump_node(tree, branch->br_first, depth+1);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		pr_indent(depth);
		printf("%4ld. %16llx: %llx\n",
			i, k->k_key, k->k_block);
		dump_node(tree, k->k_block, depth+1);
	}
}

void dump_node (
	tree_s	*tree,
	u64	blknum,
	unint	depth)
{
	buf_s	*buf;
	head_s	*h;

	buf = bget(tree->t_dev, blknum);
	if (!buf) return;

	h = buf->b_data;

	printf("blknum=%llx level=%d key=%llx\n",
		buf->b_blknum, h->h_level, h->h_key);
	switch (h->h_magic) {
	case LEAF:	dump_leaf(tree, (leaf_s *)h, depth);
			break;
	case BRANCH:	dump_branch(tree, (branch_s *)h, depth);
			break;
	default:	prmem("Unknown block magic", h, BLK_SIZE);
			break;
	}
	bput(buf);
}

void dump_tree (tree_s *tree)
{
	printf("Tree------------------------------------------------\n");
	dump_node(tree, root(tree), 0);
	fflush(stdout);
}

void verify_leaf (tree_s *tree, leaf_s *leaf, char *where)
{
	snint	sum;
	unint	i;

	if (leaf->h_magic != LEAF) {
		printf("ERROR:%s leaf=%p magic=%x\n",
			where, leaf, leaf->h_magic);
		exit(3);
	}
	if (leaf->l_total < free_space(leaf)) {
		printf("ERROR:%s allocation space bigger than total\n",
			where);
		dump_leaf(tree, leaf, 0);
		exit(3);
	}
	if (free_space(leaf) < 0) {
		printf("ERROR:%s leaf overflow\n", where);
		dump_leaf(tree, leaf, 0);
		exit(3);
	}
	sum = sizeof(leaf_s) + (leaf->l_num * sizeof(rec_s));
	for (i = 0; i < leaf->l_num; i++) {
		sum += leaf->l_rec[i].r_size;
	}
	if (BLK_SIZE - sum != leaf->l_total) {
		printf("ERROR:%s totals %ld != %d\n", where,
			BLK_SIZE - sum, leaf->l_total);
		dump_leaf(tree, leaf, 0);
		exit(3);
	}
	for (i = 1; i < leaf->l_num; i++) {
		if (leaf->l_rec[i-1].r_key >= leaf->l_rec[i].r_key) {
			printf("ERROR:%s keys out of order %llx >= %llx\n",
				where,
				leaf->l_rec[i-1].r_key, leaf->l_rec[i].r_key);
			dump_leaf(tree, leaf, 0);
			exit(3);
		}
	}
			
}

buf_s *new_leaf (tree_s *tree)
{
	buf_s	*buf;
	leaf_s	*leaf;
FN;
	buf = alloc_block(tree->t_dev);
	leaf = buf->b_data;
	leaf->h_blknum = buf->b_blknum;
	leaf->h_magic  = LEAF;
	leaf->h_level  = 0;
	leaf->h_key    = 0;
	leaf->l_end    = BLK_SIZE;
	leaf->l_total  = MAX_FREE;
	return buf;
}

buf_s *new_branch (tree_s *tree)
{
	buf_s		*buf;
	branch_s	*branch;
FN;
	buf = alloc_block(tree->t_dev);
	branch = buf->b_data;
	branch->h_blknum = buf->b_blknum;
	branch->h_magic  = BRANCH;
	branch->h_key    = 0;
	return buf;
}

void stat_node (dev_s *dev, u64 blknum, btree_stat_s *bs, unint height);

void stat_leaf (leaf_s *leaf, btree_stat_s *bs)
{
	++bs->st_num_leaves;
	bs->st_leaf_free_space += leaf->l_total;
	bs->st_leaf_recs       += leaf->l_num;
}

void stat_branch (dev_s *dev, branch_s *branch, btree_stat_s *bs, unint height)
{
	key_s	*k;
	unint	i;

	++bs->st_num_branches;
	bs->st_branch_recs += branch->br_num;

	stat_node(dev, branch->br_first, bs, height);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		stat_node(dev, k->k_block, bs, height);
	}
}

void stat_node (dev_s *dev, u64 blknum, btree_stat_s *bs, unint height)
{
	buf_s	*buf;
	head_s	*head;

	buf = bget(dev, blknum);
	if (!buf) return;

	head = buf->b_data;

	switch (head->h_magic) {
	case LEAF:	stat_leaf((leaf_s *)head, bs);
			break;
	case BRANCH:	stat_branch(dev, (branch_s *)head, bs, height + 1);
			break;
	default:	eprintf("Unknown block magic %x\n", head->h_magic);
			break;
	}
	if (height > bs->st_height) {
		bs->st_height = height;
	}
	bput(buf);
}

void stat_tree (tree_s *tree, btree_stat_s *bs)
{
	zero(*bs);
	bs->st_blk_size = BLK_SIZE;
	stat_node(tree->t_dev, root(tree), bs, 1);
}

static int compare_nodes (tree_s *a, tree_s *b, u64 a_blk, u64 b_blk);

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

static int compare_branches (tree_s *a, tree_s *b, branch_s *abranch, branch_s *bbranch)
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
		if (akey->k_block != bkey->k_block) {
			eprintf("key block %lu doesn't match %llx!=%llx\n",
				i, akey->k_block, bkey->k_block);
			return qERR_BAD_TREE;
		}
		rc = compare_nodes(a, b, akey->k_block, bkey->k_block);
		if (rc) return rc;
	}
	return 0;	
}

static int compare_nodes (tree_s *a, tree_s *b, u64 a_blk, u64 b_blk)
{
	buf_s	*abuf;
	buf_s	*bbuf;
	head_s	*ahead;
	head_s	*bhead;
	int	rc;

	abuf = bget(a->t_dev, a_blk);
	if (!abuf) {
		eprintf("compare_nodes couldn't get 'a' blk %llx", a_blk);
		return qERR_BAD_TREE;
	}
	bbuf = bget(b->t_dev, b_blk);
	if (!bbuf) {
		eprintf("compare_nodes couldn't get 'b' blk %llx", b_blk);
		return qERR_BAD_TREE;
	}
	ahead = abuf->b_data;
	bhead = bbuf->b_data;
	if (memcmp(ahead, bhead, BLK_SIZE) != 0) {
		printf("a and b are not binary identical\n");
	}

	if (ahead->h_magic != bhead->h_magic) {
		eprintf("Magic doesn't match %x!=%x\n",
			ahead->h_magic, bhead->h_magic);
		bput(abuf);
		bput(bbuf);
		return qERR_BAD_TREE;
	}
	if (ahead->h_blknum != bhead->h_blknum) {
		eprintf("Blknum doesn't match %llx!=%llx\n",
			ahead->h_blknum, bhead->h_blknum);
		bput(abuf);
		bput(bbuf);
		return qERR_BAD_TREE;
	}
	switch (ahead->h_magic) {
	case LEAF:	rc = compare_leaves(a, b, (leaf_s *)ahead, (leaf_s *)bhead);
			break;
	case BRANCH:	rc = compare_branches(a, b, (branch_s *)ahead, (branch_s *)bhead);
			break;
	default:	eprintf("compare_nodes niether leaf nor branch");
			rc = qERR_BAD_TREE;
			break;
	}
	bput(abuf);
	bput(bbuf);
	return rc;
}

int compare_trees (tree_s *a, tree_s *b)
{
	return compare_nodes(a, b, root(a), root(b));
}

rec_s *binary_search_leaf (u64 key, rec_s *keys, snint n)
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

rec_s *binary_search_leaf_or_next (u64 key, rec_s *keys, snint n)
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

snint binary_search_branch (u64 key, key_s *keys, snint n)
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
	return found(leaf, rec, key) ? rec : 0;
}

static inline rec_s *leaf_search (void *blk, u64 key)
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

static void compact (buf_s *buf)
{
	static char	block[BLK_SIZE];
	leaf_s		*leaf = buf->b_data;
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
	bdirty(buf);
}

static inline int branch_is_full (buf_s *buf)
{
	branch_s	*branch = buf->b_data;

	return branch->br_num == KEYS_PER_BRANCH;
}

static int leaf_is_full (buf_s *buf, unint size)
{
	leaf_s	*leaf = buf->b_data;
	snint	total = size + sizeof(rec_s);

	if (total <= free_space(leaf)) {
		return FALSE;
	}
	if (total > leaf->l_total) {
		return TRUE;
	}
	compact(buf);
	return FALSE;
}

buf_s *grow_tree (tree_s *tree, unint size)
{
	buf_s		*rbuf;
	buf_s		*cbuf;
	head_s		*head;
	branch_s	*branch;
	int		rc;
FN;
	if (root(tree)) {
		rbuf = bget(tree->t_dev, root(tree));
		if (!rbuf) return NULL;

		head = rbuf->b_data;
		switch (head->h_magic) {
		case LEAF:
			if (!leaf_is_full(rbuf, size)) {
				return rbuf;
			}
			break;
		case BRANCH:
			if (!branch_is_full(rbuf)) {
				return rbuf;
			}
			break;
		default:
			bput(rbuf);
			error("Bad block");
			exit(1);
			break;
		}
		cbuf = new_branch(tree);
		branch = cbuf->b_data;
		branch->br_first = rbuf->b_blknum;
		branch->h_level = head->h_level + 1;
		bput(rbuf);
		rbuf = cbuf;
	} else {
		rbuf = new_leaf(tree);
	}
	rc = change_root(tree, rbuf);
	if (rc) {
		error("Couldn't grow tree");
		return NULL;
	}
	return rbuf;
}

void insert_sibling (buf_s *pbuf, buf_s *sbuf, s64 k, u64 key)
{
	branch_s	*parent = pbuf->b_data;
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
	parent->br_key[slot].k_block = sbuf->b_blknum;
	++parent->br_num;
	bdirty(pbuf);
}

bool split_leaf (tree_s *tree, buf_s *cbuf, buf_s *pbuf, s64 k, unint size)
{
	leaf_s	*child = cbuf->b_data;
	leaf_s	*sibling;
	buf_s	*sbuf;
	snint	upper;

	if (!leaf_is_full(cbuf, size)) {
		return FALSE;
	}
FN;
	sbuf = new_leaf(tree);
	sibling = sbuf->b_data;

	upper = (child->l_num + 1) / 2;
	sibling->h_key = child->l_rec[upper].r_key;

	copy_recs(sibling, child, upper);

	child->l_num = upper;
	child->l_total += (BLK_SIZE - sizeof(leaf_s)) - sibling->l_total;
	bdirty(cbuf);
	bput(cbuf);

	insert_sibling(pbuf, sbuf, k, sibling->h_key);
	bput(sbuf);
	return TRUE;
}

bool split_branch (tree_s *tree, buf_s *cbuf, buf_s *pbuf, s64 k)
{
	branch_s	*child  = cbuf->b_data;
	branch_s	*sibling;
	buf_s		*sbuf;
	snint		upper;
	snint		midpoint;

	if (!branch_is_full(cbuf)) return FALSE;
FN;
	sbuf = new_branch(tree);
	sibling = sbuf->b_data;
	sibling->h_level = child->h_level;

	midpoint = child->br_num / 2;
	upper = midpoint + 1;

	sibling->br_first = child->br_key[midpoint].k_block;
	sibling->h_key = child->br_key[midpoint].k_key;

	memmove(sibling->br_key, &child->br_key[upper],
		sizeof(key_s) * (child->br_num - upper));
	sibling->br_num = child->br_num - upper;

	child->br_num = midpoint;
	bdirty(cbuf);
	bput(cbuf);

	insert_sibling(pbuf, sbuf, k, sibling->h_key);
	bput(sbuf);
	return TRUE;
}

static bool split_node (tree_s *tree, buf_s *cbuf, buf_s *pbuf, s64 k, unint size)
{
	switch (magic(cbuf)) {
	case LEAF:	return split_leaf(tree, cbuf, pbuf, k, size);
	case BRANCH:	return split_branch(tree, cbuf, pbuf, k);
	default:	return FALSE;
	}
}

int insert_leaf (tree_s *tree, buf_s *buf, u64 key, void *rec, unint size)
{
	leaf_s	*leaf = buf->b_data;
	rec_s	*r;
	snint	total = size + sizeof(rec_s);
FN;
	r = leaf_search(leaf, key);
	if (found(leaf, r, key)) {
		bput(buf);
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

	bdirty(buf);
	bput(buf);
	return 0;
}

static int insert_node (
	tree_s	*tree,
	buf_s	*pbuf,
	u64	key,
	void	*rec,
	unint	size);

static int insert_branch (
	tree_s	*tree,
	buf_s	*pbuf,
	u64	key,
	void	*rec,
	unint	size)
{
	buf_s		*cbuf;
	branch_s	*branch;
	s64		k;	/* Critical that this be signed */
FN;
	for (;;) {
		branch = pbuf->b_data;
		do {
			k = binary_search_branch(key, branch->br_key,
							branch->br_num);
			cbuf = bget(pbuf->b_dev, branch->br_key[k].k_block);
			if (!cbuf) {
				bput(pbuf);
				return qERR_NOT_FOUND;
			}
		} while (split_node(tree, cbuf, pbuf, k, size));
		bput(pbuf);
		if (magic(cbuf) != BRANCH) {
			return insert_node(tree, cbuf, key, rec, size);
		}
		pbuf= cbuf;
	}
}

static int insert_node (tree_s *tree, buf_s *cbuf, u64 key, void *rec, unint size)
{
FN;
	switch (magic(cbuf)) {
	case LEAF:	return insert_leaf(tree, cbuf, key, rec, size);
	case BRANCH:	return insert_branch(tree, cbuf, key, rec, size);
	default:	return qERR_BAD_BLOCK;
	}
}

int insert (tree_s *tree, u64 key, void *rec, unint size)
{
	buf_s	*cbuf;
	int	rc;
FN;
	cbuf = grow_tree(tree, size);
	rc = insert_node(tree, cbuf, key, rec, size);
	return rc;
}

int delete_leaf (tree_s *tree, buf_s *buf, u64 key)
{
	leaf_s	*leaf = buf->b_data;
	rec_s	*r;
FN;
	r = find_rec(leaf, key);
	if (r) {
		leaf->l_total += sizeof(rec_s) + r->r_size;
		memmove(r, r + 1,
			(char *)&leaf->l_rec[--leaf->l_num] - (char *)r);
		bdirty(buf);
		return 0;
	}
	printf("Key not found %llx\n", key);
	return qERR_NOT_FOUND;
}

static int join_leaf (tree_s *tree, buf_s *pbuf, buf_s *cbuf, snint k)
{
	branch_s	*parent = pbuf->b_data;
	leaf_s		*child = cbuf->b_data;
	leaf_s		*sibling;
	buf_s		*sbuf;

	sbuf = bget(tree->t_dev, parent->br_key[k+1].k_block);
	if (!sbuf) return qERR_NOT_FOUND;

	sibling = sbuf->b_data;

	if (child->l_total + sibling->l_total > MAX_FREE) {
HERE;
		compact(cbuf);
// Don't need to do this		compact(sbuf);
		copy_recs(child, sibling, 0);
		bdirty(cbuf);
		memmove( &parent->br_key[k+1], &parent->br_key[k+2],
			sizeof(parent->br_key[0]) * (parent->br_num - (k+2)));
		--parent->br_num;
		bdirty(pbuf);
		bdirty(sbuf);
	}
	//verify_leaf(child, WHERE);
	bput(sbuf);	// Should free sibling
	return 0;
}

static int join_branch (tree_s *tree, buf_s *pbuf, buf_s *cbuf, snint k)
{
	branch_s	*parent = pbuf->b_data;
	branch_s	*child = cbuf->b_data;
	branch_s	*sibling;
	buf_s		*sbuf;

	sbuf = bget(tree->t_dev, parent->br_key[k+1].k_block);
	if (!sbuf) {
		return qERR_NOT_FOUND;
	}
	sibling = sbuf->b_data;
	if (child->br_num+sibling->br_num < KEYS_PER_BRANCH-1) {
HERE;
		child->br_key[child->br_num].k_key = parent->br_key[k+1].k_key;
		child->br_key[child->br_num].k_block = sibling->br_first;
		++child->br_num;
		memmove( &child->br_key[child->br_num], sibling->br_key,
			sibling->br_num * sizeof(sibling->br_key[0]));
		child->br_num += sibling->br_num;
		bdirty(cbuf);

		memmove( &parent->br_key[k+1], &parent->br_key[k+2],
			sizeof(parent->br_key[0]) * (parent->br_num - (k+2)));
		--parent->br_num;
		bdirty(sbuf);
		bdirty(pbuf);
	}
	bput(sbuf);	// Sibling should be freed.
	return 0;
}

static int join_node (tree_s *tree, buf_s *pbuf, snint k)
{
	branch_s	*parent = pbuf->b_data;
	head_s		*child;
	buf_s		*cbuf;
	int		rc;
FN;
	if (k == parent->br_num - 1) {
		/* no sibling to the right */
		return 0;
	}
	cbuf = bget(tree->t_dev, parent->br_key[k].k_block);
	if (!cbuf) return qERR_NOT_FOUND;

	child = cbuf->b_data;
	switch (child->h_magic) {
	case LEAF:	rc = join_leaf(tree, pbuf, cbuf, k);
			break;
	case BRANCH:	rc = join_branch(tree, pbuf, cbuf, k);
			break;
	default:	rc = qERR_BAD_BLOCK;
			break;
	}
	bput(cbuf);
	return rc;
}

int delete (tree_s *tree, u64 key)
{
	buf_s		*cbuf;
	buf_s		*pbuf;
	head_s		*child;
	branch_s	*parent;
	snint		k;
	int		rc;
FN;
	cbuf = bget(tree->t_dev, root(tree));
	for (;;) {
		if (!cbuf) return qERR_NOT_FOUND;

		child = cbuf->b_data;
		if (child->h_magic == LEAF) {
			break;
		}
		pbuf = cbuf;
		parent = (branch_s *)child;
		k = binary_search_branch(key, parent->br_key, parent->br_num);
		join_node(tree, pbuf, k);
		cbuf = bget(tree->t_dev, parent->br_key[k].k_block);
		bput(pbuf);
	}
	rc = delete_leaf(tree, cbuf, key);
	bput(cbuf);
	return rc;
}

int find_or_next_leaf (
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

int search_leaf (
	tree_s		*tree,
	buf_s		*buf,
	u64		key,
	search_f	sf,
	void		*data)
{
	leaf_s	*leaf = buf->b_data;
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

int search_node (tree_s *tree, buf_s *buf, u64 key, search_f sf, void *data);

int search_branch (
	tree_s		*tree,
	buf_s		*pbuf,
	u64		key,
	search_f	sf,
	void		*data)
{
	branch_s	*branch = pbuf->b_data;
	buf_s		*cbuf;
	snint		k;
	int		rc;

	k = binary_search_branch(key, branch->br_key, branch->br_num);
	do {
		cbuf = bget(pbuf->b_dev, branch->br_key[k].k_block);
		if (!cbuf) return qERR_NOT_FOUND;

		rc = search_node(tree, cbuf, key, sf, data);
		bput(cbuf);
		if (rc != qERR_TRY_NEXT) {
			return rc;
		}
		++k;
	} while (k != branch->br_num);
	return rc;
}

int search_node (tree_s *tree, buf_s *buf, u64 key, search_f sf, void *data)
{
FN;
	switch (magic(buf)) {
	case LEAF:	return search_leaf(tree, buf, key, sf, data);
	case BRANCH:	return search_branch(tree, buf, key, sf, data);
	default:	return qERR_BAD_BLOCK;
	}
}

int search (
	tree_s		*tree,
	u64		key,
	search_f	sf,
	void		*data)
{
	buf_s		*buf;
	int		rc;
FN;
	buf = bget(tree->t_dev, root(tree));
	if (!buf) return qERR_NOT_FOUND;

	rc = search_node(tree, buf, key, sf, data);
	bput(buf);

	return rc;
}
