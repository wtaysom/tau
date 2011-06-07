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

typedef enum { LEAF = 1, BRANCH, END } block_t;

typedef struct key_s {
	u64	k_key;
	u64	k_block;
} key_s;

typedef struct rec_s {
	s16	r_start;
	s16	r_len;
	u16	r_reserved[2];
	u64	r_key;
} rec_s;

typedef struct head_s {
	unsigned short	h_type;
	unsigned short	h_reserved[3];
} head_s;

/* b0 k1 b1 k2 b2 k3 b3 */
typedef struct branch_s {
	u16	br_type;
	s16	br_num;
	u16	br_reserved[2];
	u64	br_first;
	key_s	br_key[0];
} branch_s;

typedef struct leaf_s {
	u16	l_type;
	s16	l_num;
	s16	l_end;
	s16	l_total;	// change to free later
	// Another way to make end work would be to use
	// &l_rec[l_num] as the index point of l_end
	// That is what I did in insert!!!
	rec_s	l_rec[0];
} leaf_s;

enum {
	KEYS_PER_BRANCH = (BLK_SIZE - sizeof(branch_s)) / sizeof(key_s),
	MAX_FREE = BLK_SIZE - sizeof(leaf_s)
};

void dump_block (
	tree_s	*tree,
	u64	blkno,
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

static inline unint type (buf_s *buf)
{
	head_s	*h;

	h = buf->b_data;
	return h->h_type;
}

/*
 *	+--------+------+------------+------------------+
 *	| leaf_s | recs | free space | value of records |
 *	+--------+------+------------+------------------+
 *	         ^      ^            ^
 *	         |l_num |           l_end
 */
static inline int free_space (leaf_s *leaf)
{
	return leaf->l_end - (leaf->l_num * sizeof(rec_s) + sizeof(leaf_s));
}

void dump_leaf (
	tree_s	*tree,
	leaf_s	*leaf,
	unint	indent)
{
	rec_s	*end = &leaf->l_rec[leaf->l_num];
	rec_s	*r;
	char	*c;
	unsigned	i;
	//unsigned	j;

//FN;
	if (!leaf) return;

	pr_indent(indent);
	printf("leaf: num keys = %d end = %d total = %d current = %u\n",
			leaf->l_num, leaf->l_end, leaf->l_total,
			free_space(leaf));
	fflush(stdout);
	for (r = leaf->l_rec, i = 0; r < end; r++, i++) {
		c = (char *)leaf + r->r_start;
		pr_indent(indent);
		printf("\t%4d. %16llx %4d:%4d ",
			i, r->r_key, r->r_start, r->r_len);

		dump_rec(tree, r->r_key, c, r->r_len);

		printf("\n");
		fflush(stdout);
	}
	fflush(stdout);
}

void dump_branch (
	tree_s		*tree,
	branch_s	*branch,
	unint		indent)
{
	key_s		*k;
	unsigned	i;

//FN;
	if (!branch) return;

	pr_indent(indent);
	printf("branch: num keys = %d\n",
		branch->br_num);
	pr_indent(indent);
	printf("first %llx\n", branch->br_first);
	dump_block(tree, branch->br_first, indent+1);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		pr_indent(indent);
		printf("%4d. %16llx: %llx\n",
			i, k->k_key, k->k_block);
		dump_block(tree, k->k_block, indent+1);
	}
}

void dump_block (
	tree_s	*tree,
	u64	blkno,
	unint	indent)
{
	buf_s	*buf;
	head_s	*b;

//FN;
	buf = bget(tree->t_dev, blkno);
	if (!buf) return;

	b = buf->b_data;

	switch (b->h_type) {
	case LEAF:	dump_leaf(tree, (leaf_s *)b, indent);
			break;
	case BRANCH:	dump_branch(tree, (branch_s *)b, indent);
			break;
	default:	prmem("Unknown block type", b, BLK_SIZE);
			break;
	}
	bput(buf);
}

void dump_tree (tree_s *tree)
{
FN;
	printf("Tree------------------------------------------------\n");
	dump_block(tree, root(tree), 0);
	fflush(stdout);
}

void verify_leaf (tree_s *tree, leaf_s *leaf, char *where)
{
	int	sum;
	int	i;

	if (leaf->l_type != LEAF) {
		printf("ERROR:%s leaf=%p type=%d\n",
			where, leaf, leaf->l_type);
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
		sum += leaf->l_rec[i].r_len;
	}
	if (BLK_SIZE - sum != leaf->l_total) {
		printf("ERROR:%s totals %d != %d\n", where,
			BLK_SIZE - sum, leaf->l_total);
		dump_leaf(tree, leaf, 0);
		exit(3);
	}
	for (i = 1; i < leaf->l_num; i++) {
		if (leaf->l_rec[i-1].r_key >= leaf->l_rec[i].r_key) {
			printf("ERROR:%s keys out of order %llu >= %llu\n",
				where,
				leaf->l_rec[i-1].r_key, leaf->l_rec[i].r_key);
			dump_leaf(tree, leaf, 0);
			exit(3);
		}
	}

}

buf_s *new_leaf (dev_s *dev)
{
	buf_s	*buf;
	leaf_s	*leaf;
FN;
	buf = alloc_block(dev);
	leaf = buf->b_data;
	leaf->l_type = LEAF;
	leaf->l_end  = BLK_SIZE;
	leaf->l_total = MAX_FREE;
	return buf;
}

buf_s *new_branch (dev_s *dev)
{
	buf_s		*buf;
	branch_s	*branch;
FN;
	buf = alloc_block(dev);
	branch = buf->b_data;
	branch->br_type = BRANCH;
	return buf;
}

void stat_block (dev_s *dev, u64 blkno, btree_stat_s *bs, int height);

void stat_leaf (leaf_s *leaf, btree_stat_s *bs)
{
	++bs->st_num_leaves;
	bs->st_leaf_free_space += leaf->l_total;
	bs->st_leaf_recs       += leaf->l_num;
}

void stat_branch (dev_s *dev, branch_s *branch, btree_stat_s *bs, int height)
{
	key_s		*k;
	unsigned	i;

	++bs->st_num_branches;
	bs->st_branch_recs += branch->br_num;

	stat_block(dev, branch->br_first, bs, height);
	for (k = branch->br_key, i = 0; i < branch->br_num; k++, i++) {
		stat_block(dev, k->k_block, bs, height);
	}
}

void stat_block (dev_s *dev, u64 blkno, btree_stat_s *bs, int height)
{
	buf_s	*buf;
	head_s	*blk;

	buf = bget(dev, blkno);
	if (!buf) return;

	blk = buf->b_data;

	switch (blk->h_type) {
	case LEAF:	stat_leaf((leaf_s *)blk, bs);
			break;
	case BRANCH:	stat_branch(dev, (branch_s *)blk, bs, height + 1);
			break;
	default:	eprintf("Unknown block type %d\n", blk->h_type);
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
	stat_block(tree->t_dev, root(tree), bs, 1);
}

rec_s *binary_search_leaf (u64 key, rec_s *keys, int n)
{
	int	x, left, right;	/* left and right must be
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

rec_s *binary_search_leaf_or_next (u64 key, rec_s *keys, int n)
{
	int	x, left, right;	/* left and right must be
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

unsigned binary_search_branch (u64 key, key_s *keys, int n)
{
	int	x, left, right;	/* left and right must be
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

void copy_recs (leaf_s *dst, leaf_s *src, unsigned start)
{
	rec_s		*s;
	rec_s		*d;
	unsigned	i;
FN;
	for (i = start; i < src->l_num; i++) {
		s = &src->l_rec[i];
		d = &dst->l_rec[dst->l_num++];
		dst->l_total -= s->r_len + sizeof(rec_s);
		dst->l_end  -= s->r_len;
		d->r_start   = dst->l_end;
		d->r_key     = s->r_key;
		d->r_len     = s->r_len;
		memmove((char *)dst + d->r_start,
			(char *)src + s->r_start,
			s->r_len);
	}
}

void compact (buf_s *buf)
{
	static char	block[BLK_SIZE];
	leaf_s		*leaf = buf->b_data;
	leaf_s		*p;
FN;
	// Need a spin lock here
	p = (leaf_s *)block;
	bzero(p, BLK_SIZE);
	p->l_type = leaf->l_type;
	p->l_end  = BLK_SIZE;
	p->l_total = MAX_FREE;

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

static int leaf_is_full (buf_s *buf, unint len)
{
	leaf_s	*leaf = buf->b_data;
	int	total = len + sizeof(rec_s);

	if (total <= free_space(leaf)) {
		return FALSE;
	}
	if (total > leaf->l_total) {
		return TRUE;
	}
	compact(buf);
	return FALSE;
}

buf_s *grow_tree (tree_s *tree, unint len)
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
		switch (head->h_type) {
		case LEAF:
			if (!leaf_is_full(rbuf, len)) {
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
		cbuf = new_branch(tree->t_dev);
		branch = cbuf->b_data;
		branch->br_first = rbuf->b_blkno;
		bput(rbuf);
		rbuf = cbuf;
	} else {
		rbuf = new_leaf(tree->t_dev);
	}
	rc = change_root(tree, rbuf);
	if (rc) {
		error("Couldn't grow tree");
		return NULL;
	}
	return rbuf;
}

void insert_sibling (buf_s *pbuf, buf_s *sbuf, int k, u64 key)
{
	branch_s	*parent = pbuf->b_data;
	int		slot;
FN;
	/* make a hole -- only if not at end */
	slot = k + 1;
	if (slot < parent->br_num) {
		memmove( &parent->br_key[slot+1],
			 &parent->br_key[slot],
			 sizeof(key_s) * (parent->br_num - slot));
	}
	parent->br_key[slot].k_key   = key;
	parent->br_key[slot].k_block = sbuf->b_blkno;
	++parent->br_num;
	bdirty(pbuf);
}

bool split_leaf (void *self, buf_s *pbuf, int k, unint len)
{
	buf_s	*cbuf = self;
	leaf_s	*child = cbuf->b_data;
	leaf_s	*sibling;
	buf_s	*sbuf;
	int	upper;
	u64	upper_key;
FN;
	if (!leaf_is_full(cbuf, len)) {
		return FALSE;
	}
	sbuf = new_leaf(pbuf->b_dev);
	sibling = sbuf->b_data;

	upper = (child->l_num + 1) / 2;
	upper_key = child->l_rec[upper].r_key;

	copy_recs(sibling, child, upper);

	child->l_num = upper;
	child->l_total += (BLK_SIZE - sizeof(leaf_s)) - sibling->l_total;
	bdirty(cbuf);
	bput(cbuf);

	insert_sibling(pbuf, sbuf, k, upper_key);
	bput(sbuf);
	return TRUE;
}

bool split_branch (void *self, buf_s *pbuf, int k)
{
	buf_s		*cbuf = self;
	branch_s	*child  = cbuf->b_data;
	branch_s	*sibling;
	buf_s		*sbuf;
	int		upper;
	int		midpoint;
	u64		midpoint_key;
FN;
	if (!branch_is_full(cbuf)) return FALSE;

	sbuf = new_branch(cbuf->b_dev);
	sibling = sbuf->b_data;

	midpoint = child->br_num / 2;
	upper = midpoint + 1;

	sibling->br_first = child->br_key[midpoint].k_block;
	midpoint_key      = child->br_key[midpoint].k_key;

	memmove(sibling->br_key, &child->br_key[upper],
		sizeof(key_s) * (child->br_num - upper));
	sibling->br_num = child->br_num - upper;

	child->br_num = midpoint;
	bdirty(cbuf);
	bput(cbuf);

	insert_sibling(pbuf, sbuf, k, midpoint_key);
	bput(sbuf);
	return TRUE;
}

bool split (buf_s *cbuf, buf_s *pbuf, int k, unint len)
{
	switch (type(cbuf)) {
	case LEAF:	return split_leaf(cbuf, pbuf, k, len);
	case BRANCH:	return split_branch(cbuf, pbuf, k);
	default:	return FALSE;
	}
}

int insert_leaf (tree_s *tree, buf_s *buf, u64 key, void *rec, unint len)
{
	leaf_s	*leaf = buf->b_data;
	rec_s	*r;
	int	total = len + sizeof(rec_s);
FN;
	r = leaf_search(leaf, key);
	if (found(leaf, r, key)) {
		bput(buf);
		return qERR_DUP;
	}
	memmove(r + 1, r, (char *)&leaf->l_rec[leaf->l_num] - (char *)r);
	++leaf->l_num;
	leaf->l_total -= total;
	leaf->l_end -= len;
	r->r_start = leaf->l_end;
	r->r_len   = len;
	r->r_key   = key;
	pack_rec(tree, (u8 *)leaf + r->r_start, rec, len);
	bdirty(buf);
	bput(buf);
	return 0;
}

static int insert_head (
	tree_s	*tree,
	buf_s	*pbuf,
	u64	key,
	void	*rec,
	unint	len);

static int insert_branch (
	tree_s	*tree,
	buf_s	*pbuf,
	u64	key,
	void	*rec,
	unint	len)
{
	buf_s		*cbuf;
	branch_s	*branch;
	int		k;	/* Critical that this be signed */
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
		} while (split(cbuf, pbuf, k, len));
		bput(pbuf);
		if (type(cbuf) != BRANCH) {
			return insert_head(tree, cbuf, key, rec, len);
		}
		pbuf= cbuf;
	}
}

static int insert_head (tree_s *tree, buf_s *cbuf, u64 key, void *rec, unint len)
{
FN;
	switch (type(cbuf)) {
	case LEAF:	return insert_leaf(tree, cbuf, key, rec, len);
	case BRANCH:	return insert_branch(tree, cbuf, key, rec, len);
	default:	return qERR_BAD_BLOCK;
	}
}

int insert (tree_s *tree, u64 key, void *rec, unint len)
{
	buf_s	*cbuf;
FN;
	cbuf = grow_tree(tree, len);
	return insert_head(tree, cbuf, key, rec, len);
}


void delete_leaf (buf_s *buf, u64 key)
{
	leaf_s	*leaf = buf->b_data;
	rec_s	*r;
FN;
	r = find_rec(leaf, key);
	if (r) {
		leaf->l_total += sizeof(rec_s) + r->r_len;
		memmove(r, r + 1,
			(char *)&leaf->l_rec[--leaf->l_num] - (char *)r);
		bdirty(buf);
		return;
	}
	printf("Key not found %llu\n", key);
	exit(2);
}

static int join_leaf (tree_s *tree, buf_s *pbuf, buf_s *cbuf, int k)
{
	branch_s	*parent = pbuf->b_data;
	leaf_s		*child = cbuf->b_data;
	leaf_s		*sibling;
	buf_s		*sbuf;

	sbuf = bget(tree->t_dev, parent->br_key[k+1].k_block);
	if (!sbuf) return qERR_NOT_FOUND;

	sibling = sbuf->b_data;

	if (child->l_total + sibling->l_total > MAX_FREE) {
FN;
		compact(cbuf);
		compact(sbuf);
		copy_recs(child, sibling, 0);
		memmove( &parent->br_key[k+1], &parent->br_key[k+2],
			sizeof(parent->br_key[0]) * (parent->br_num - (k+2)));
		--parent->br_num;
		bdirty(pbuf);
	}
	//verify_leaf(child, WHERE);
	bput(sbuf);	// Should free sibling
	return 0;
}

static int join_branch (tree_s *tree, buf_s *pbuf, buf_s *cbuf, int k)
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
FN;
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
		bdirty(pbuf);
	}
	bput(sbuf);	// Sibling should be freed.
	return 0;
}

static int join (tree_s *tree, buf_s *pbuf, int k)
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
	switch (child->h_type) {
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
	int		k;
FN;
	cbuf = bget(tree->t_dev, root(tree));
	for (;;) {
		if (!cbuf) return qERR_NOT_FOUND;

		child = cbuf->b_data;
		if (child->h_type == LEAF) {
			break;
		}
		pbuf = cbuf;
		parent = (branch_s *)child;
		k = binary_search_branch(key, parent->br_key, parent->br_num);
		join(tree, pbuf, k);
		cbuf = bget(tree->t_dev, parent->br_key[k].k_block);
		bput(pbuf);
	}
	delete_leaf(cbuf, key);
	bput(cbuf);
	return 0;
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
			rec->r_len);
	}
	return 0;
}


int search_leaf (
	void		*self,
	u64		key,
	search_f	sf,
	void		*data)
{
	buf_s	*buf = self;
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
			(char *)leaf + rec->r_start, rec->r_len);
		if (rc != qERR_TRY_NEXT) {
			return rc;
		}
		++rec;
	}
}

int search_head (buf_s *buf, u64 key, search_f sf, void *data);

int search_branch (
	void		*self,
	u64		key,
	search_f	sf,
	void		*data)
{
	buf_s		*pbuf = self;
	branch_s	*branch = pbuf->b_data;
	buf_s		*cbuf;
	int		k;
	int		rc;

	k = binary_search_branch(key, branch->br_key, branch->br_num);
	do {
		cbuf = bget(pbuf->b_dev, branch->br_key[k].k_block);
		if (!cbuf) return qERR_NOT_FOUND;

		rc = search_head(cbuf, key, sf, data);
		bput(cbuf);
		if (rc != qERR_TRY_NEXT) {
			return rc;
		}
		++k;
	} while (k != branch->br_num);
	return rc;
}

int search_head (buf_s *buf, u64 key, search_f sf, void *data)
{
FN;
	switch (type(buf)) {
	case LEAF:	return search_leaf(buf, key, sf, data);
	case BRANCH:	return search_branch(buf, key, sf, data);
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

	rc = search_head(buf, key, sf, data);
	bput(buf);

	return rc;
}
