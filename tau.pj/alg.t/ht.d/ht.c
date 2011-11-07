/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

/*
 * Terminology:
 * lump - a pointer with a size. Nil is <0, NULL>. Notice that
 *  lumps are passed by value which means the size and
 *  the pointer are copied onto the stack.
 * key - a unsiged number (some times a hash). Index for a record.
 * val - a lump. Value of a record.
 * rec - <key, val>
 * twig - <key, blknum>
 * leaf - where the records of a the B-tree are stored.
 * branch - Store indexes to to leaves on other branches
 */

// Put static in front of everything

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>

#include "ht.h"
#include "ht_disk.h"
#include "buf.h"

#define LF_AUDIT(_h) lf_audit(FN_ARG, _h)
#define BR_AUDIT(_h) br_audit(FN_ARG, _h)

enum {	MAX_U16 = (1 << 16) - 1,
	BITS_U8 = 8,
	MASK_U8 = (1 << BITS_U8) - 1,
	SZ_U16  = sizeof(u16),
	SZ_HEAD = sizeof(Node_s),
	LEAF_OVERHEAD = 2 * SZ_U16 + sizeof(Key_t),
	REBALANCE = 2 * (BLOCK_SIZE - SZ_HEAD) / 3 };

struct Htree_s {
	Cache_s	*cache;
	u64	root;
	Stat_s	stat;
};

typedef struct Apply_s {
	Apply_f	func;
	Htree_s	*tree;
	void	*sys;
	void	*user;
} Apply_s;

void lump_dump(Lump_s a);
void lf_dump(Buf_s *buf, int indent);
void br_dump(Buf_s *buf, int indent);
void node_dump(Htree_s *t, Blknum_t blknum, int indent);
void pr_leaf(Node_s *leaf);
void pr_branch(Node_s *branch);

bool Dump_buf = FALSE;

static inline Apply_s mk_apply (Apply_f func, Htree_s *t, void *sys, void *user)
{
	Apply_s a;

	a.func = func;
	a.tree = t;
	a.sys  = sys;
	a.user = user;
	return a;
}

void cache_err (Htree_s *t)
{
	CacheStat_s cs = cache_stats(t->cache);
	printf("cache stats:\n"
		"  num bufs = %8d\n"
		"  gets     = %8lld\n"
		"  puts     = %8lld\n"
		"  hits     = %8lld\n"
		"  miss     = %8lld\n"
		"hit ratio  = %8g%%\n",
		cs.numbufs,
		cs.gets, cs.puts, cs.hits, cs.miss,
		(double)(cs.hits) / (cs.hits + cs.miss) * 100.);
}

Buf_s *t_get (Htree_s *t, Blknum_t blknum)
{
	Buf_s *buf = buf_get(t->cache, blknum);
	buf->user = t;
	return buf;
}

Stat_s      t_get_stats   (Htree_s *t) { return t->stat; }
CacheStat_s t_cache_stats (Htree_s *t) { return cache_stats(t->cache); }

int usable (Node_s *leaf)
{
FN;
	assert(leaf->isleaf);
	return leaf->end - SZ_HEAD - SZ_U16 * leaf->numrecs;
}

int inuse (Node_s *leaf)
{
	return BLOCK_SIZE - SZ_HEAD - leaf->free;
}

void pr_indent (int indent)
{
	int i;

	for (i = 0; i < indent; i++) {
		printf("  ");
	}
}

void pr_head (Node_s *node, int indent)
{
	pr_indent(indent);
	if (node->isleaf) {
		printf("LEAF %lld numrecs: %d free: %d end: %d\n",
			node->blknum,
			node->numrecs,
			node->free,
			node->end);
	} else {
		printf("BRANCH %lld numrecs: %d first: %lld\n",
			node->blknum,
			node->numrecs,
			node->first);
	}
}

void pr_buf(Buf_s *buf, int indent)
{
	enum { BYTES_PER_ROW = 16 };
	unint	i;
	unint	j;
	u8	*d = buf->d;
	char	*c = buf->d;

	pr_indent(indent);
	printf("blknum=%lld\n", buf->blknum);
	for (i = 0; i < BLOCK_SIZE / BYTES_PER_ROW; i++) {
		pr_indent(indent);
		for (j = 0; j < BYTES_PER_ROW; j++) {
			printf(" %.2x", *d++);
		}
		printf(" | ");
		for (j = 0; j < BYTES_PER_ROW; j++) {
			putchar(isprint(*c) ? *c : '.');
			++c;
		}
		printf("\n");
	}
}

static void pr_chars (int n, u8 *a)
{
	int	i;

	for (i = 0; i < n; i++) {
		if (isprint(a[i])) {
			putchar(a[i]);
		} else {
			putchar('.');
		}
	}

}

static void pr_key (Key_t key)
{
	printf("%llu", (u64)key);
}

static void pr_lump (Lump_s a)
{
	enum { MAX_PRINT = 32 };
	int	size;

	size = a.size;
	if (size > MAX_PRINT) {
		size = MAX_PRINT;
		pr_chars(MAX_PRINT/2 - 3, a.d);
		printf(" ... ");
		pr_chars(MAX_PRINT/2 - 2, &a.d[MAX_PRINT/2 + 2]);
	} else {
		pr_chars(size, a.d);
	}
}

void pr_rec (Hrec_s rec)
{
	pr_key(rec.key);
	printf(" = ");
	pr_lump(rec.val);
}

void init_node (Node_s *node, bool isleaf, Blknum_t blknum)
{
FN;
	node->isleaf = isleaf;
	node->numrecs = 0;
	node->blknum  = blknum;
	if (isleaf) {
		node->free  = BLOCK_SIZE - SZ_HEAD;
		node->end   = BLOCK_SIZE;
	} else {
		node->first = 0;
	}
}

Key_t get_key (Node_s *leaf, unint i)
{
	Key_t	key;
	unint	x;
	u8	*start;

	assert(leaf->isleaf);
	assert(i < leaf->numrecs);
	x = leaf->rec[i];
	assert(x < BLOCK_SIZE);
	start = &((u8 *)leaf)[x];
	UNPACK(start, key);
	return key;
}

Lump_s get_val (Node_s *leaf, unint i)
{
	Lump_s	val;
	unint	x;
	u8	*start;

	assert(leaf->isleaf);
	assert(i < leaf->numrecs);
	x = leaf->rec[i];
	assert(x < BLOCK_SIZE);
	start = &((u8 *)leaf)[x];
	start += sizeof(Key_t);
	val.size = *start++;
	val.size |= (*start++) << 8;
	val.d = start;
	return val;
}

Hrec_s get_rec (Node_s *leaf, unint i)
{
	Hrec_s	rec;
	unint	x;
	u8	*start;

	assert(leaf->isleaf);
	assert(i < leaf->numrecs);
	x = leaf->rec[i];
	assert(x < BLOCK_SIZE);
	start = &((u8 *)leaf)[x];
	
	UNPACK(start, rec.key);
	start += sizeof(Key_t);
	rec.val.size = *start++;
	rec.val.size |= (*start++) << 8;
	rec.val.d = start;
	return rec;
}

void key_dump (Key_t key)
{
	printf("%llu", (u64)key);
}

void lump_dump(Lump_s a)
{
#if 0
	int	i;

	for (i = 0; i < a.size; i++) {
		putchar(a.d[i]);
	}
#else
	printf("%.*s", a.size, a.d);
#endif
}

void rec_dump (Hrec_s rec)
{
	key_dump(rec.key);
	printf(" = ");
	lump_dump(rec.val);
	printf("\n");
}

void lf_dump (Buf_s *buf, int indent)
{
	Node_s	*leaf = buf->d;
	Hrec_s	rec;
	unint	i;

	if (Dump_buf) {
		pr_buf(buf, indent);
	}
	pr_head(leaf, indent);
	for (i = 0; i < leaf->numrecs; i++) {
		rec = get_rec(leaf, i);
		pr_indent(indent);
		printf("%ld. ", i);
		rec_dump(rec);
	}
}

void br_dump (Buf_s *buf, int indent)
{
	Node_s	*branch = buf->d;
	Twig_s	twig;
	unint	i;

	pr_head(branch, indent);
	pr_indent(indent);
	printf("<first> = %lld\n", (u64)branch->first);
	node_dump(buf->user, branch->first, indent + 1);
	for (i = 0; i < branch->numrecs; i++) {
		twig = branch->twig[i];
		pr_indent(indent);
		printf("%ld. ", i);
		key_dump(twig.key);
		printf(" = %lld\n", twig.blknum);
		node_dump(buf->user, twig.blknum, indent + 1);
	}
}

void node_dump (Htree_s *t, Blknum_t blknum, int indent)
{
	Buf_s	*buf;
	Node_s	*node;

	if (!blknum) return;
	buf = t_get(t, blknum);
	node = buf->d;
	if (node->isleaf) {
		lf_dump(buf, indent);
	} else {
		br_dump(buf, indent + 1);
	}
	buf_put( &buf);
}

#if 0
bool isGE (Node_s *leaf, Key_t key, unint i)
{
FN;
	Key_t	target;

	target = get_key(leaf, i);
	return target >= key;
}
#endif

bool isLT (Node_s *leaf, Key_t key, unint i)
{
	Key_t	target;

	target = get_key(leaf, i);
	return key < target;
}

int isEQ (Node_s *leaf, Key_t key, unint i)
{
	Key_t	target;

	target = get_key(leaf, i);
	if (target < key) return -1;
	if (target == key) return 0;
	return 1;
}

void store_key (Node_s *leaf, Key_t key)
{
FN;
	u8	*start;

	assert(leaf->isleaf);
	assert(sizeof(Key_t) <= leaf->free);
	assert(sizeof(Key_t) <= leaf->end);

	leaf->end -= sizeof(Key_t);
	leaf->free -= sizeof(Key_t);
	start = &((u8 *)leaf)[leaf->end];
	PACK(start, key);
}

void store_lump (Node_s *leaf, Lump_s lump)
{
FN;
	int total = lump.size + SZ_U16;
	u8 *start;

	assert(leaf->isleaf);
	assert(total <= leaf->free);
	assert(total <= leaf->end);
	assert(lump.size < MAX_U16);

	leaf->end -= total;
	leaf->free -= total;
	start = &((u8 *)leaf)[leaf->end];
	*start++ = lump.size & MASK_U8;
	*start++ = (lump.size >> BITS_U8) & MASK_U8;
	memmove(start, lump.d, lump.size);
}

void update_blknum (Node_s *branch, Blknum_t blknum, unint irec)
{
FN;
	assert(!branch->isleaf);
	assert(irec < branch->numrecs);
	branch->twig[irec].blknum = blknum;
}

/* XXX: Don't like this name */
void store_end (Node_s *leaf, unint i)
{
FN;
	assert(leaf->isleaf);
	assert(i <= leaf->numrecs);
	if (i < leaf->numrecs) {
		memmove( &leaf->rec[i+1], &leaf->rec[i],
			(leaf->numrecs - i) * SZ_U16);
	}
	++leaf->numrecs;
	leaf->rec[i] = leaf->end;
	leaf->free -= SZ_U16;
}

void rec_copy(Node_s *dst, int a, Node_s *src, int b)
{
	assert(dst->isleaf);
	assert(src->isleaf);

	Hrec_s rec = get_rec(src, b);

	store_lump(dst, rec.val);
	store_key(dst, rec.key);
	store_end(dst, a);
}

void twig_copy (Node_s *dst, int a, Node_s *src, int b)
{
	assert(!dst->isleaf);
	assert(!src->isleaf);

	if (a < dst->numrecs) {
		memmove( &dst->twig[a + 1], &dst->twig[a],
			(dst->numrecs - a) * SZ_U16);
	}
	dst->twig[a] = src->twig[b];
	++dst->numrecs;
}

void lf_compact (Node_s *leaf)
{
FN;
	u8	b[BLOCK_SIZE];
	Node_s	*h = (Node_s *)b;
	int	i;

	if (usable(leaf) == leaf->free) {
		return;
	}
	init_node(h, TRUE, leaf->blknum);
	for (i = 0; i < leaf->numrecs; i++) {
		rec_copy(h, i, leaf, i);
	}
	memmove(leaf, h, BLOCK_SIZE);
}

void store_rec (Node_s *leaf, Key_t key, Lump_s val, unint i)
{
FN;
	assert(leaf->isleaf);
	lf_compact(leaf);	// XXX: not sure if needed and expensive
	store_lump(leaf, val);
	store_key(leaf, key);
	store_end(leaf, i);
}

void delete_rec (Node_s *leaf, unint i)
{
FN;
	Hrec_s	rec;

	assert(i < leaf->numrecs);
	assert(leaf->isleaf);
	rec = get_rec(leaf, i);
	leaf->free += rec.val.size + LEAF_OVERHEAD;
	--leaf->numrecs;
	if (i < leaf->numrecs) {
		memmove( &leaf->rec[i], &leaf->rec[i+1],
			(leaf->numrecs - i) * SZ_U16);
	}
}

void lf_audit (const char *fn, unsigned line, Node_s *leaf)
{
FN;
	Hrec_s	rec;
	int	free = BLOCK_SIZE - SZ_HEAD;
	int	i;

	assert(leaf->isleaf);
	for (i = 0; i < leaf->numrecs; i++) {
		rec = get_rec(leaf, i);
		free -= rec.val.size + LEAF_OVERHEAD;
	}
	if (free != leaf->free) {
		printf("%s<%d> free:%d != %d:leaf->free\n",
			fn, line, free, leaf->free);
		exit(2);
	}
}

void br_audit (const char *fn, unsigned line, Node_s *branch)
{
FN;
	Key_t	prev;
	Key_t	key;
	int	i;

	assert(!branch->isleaf);

	prev = 0;
	for (i = 0; i < branch->numrecs; i++) {
		key = branch->twig[i].key;
		if (key < prev) {
			fatal("Key out of order prev = %lld key = %lld"
				" branch = %p block = %lld\n",
				(u64)key, (u64)key, branch,
				(u64)branch->blknum);
		}
	}
}

int leaf_lt (Node_s *leaf, Key_t key)
{
FN;
	int	x;
	int	left;
	int	right;

	assert(leaf->isleaf);
	left = 0;
	right = leaf->numrecs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (isLT(leaf, key, x)) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return left;
}

int br_lt (Node_s *branch, Key_t key)
{
FN;
	int	x;
	int	left;
	int	right;

	left = 0;
	right = branch->numrecs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (key < branch->twig[x].key) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return left;
}

int leaf_eq (Node_s *leaf, Key_t key)
{
FN;
	int	x;
	int	left;
	int	right;
	int	eq;

	left = 0;
	right = leaf->numrecs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		eq = isEQ(leaf, key, x);
		if (eq == 0) {
			return x;
		} else if (eq > 0) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	if (left == leaf->numrecs) return left;
	return -1;
}

void lf_del_rec (Node_s *leaf, unint i)
{
FN;
	Hrec_s	rec;

	LF_AUDIT(leaf);
	assert(i < leaf->numrecs);
	rec = get_rec(leaf, i);
	leaf->free += rec.val.size + LEAF_OVERHEAD;

	--leaf->numrecs;
	if (i < leaf->numrecs) {
		memmove( &leaf->rec[i], &leaf->rec[i + 1],
			(leaf->numrecs - i) * SZ_U16);
	}
	LF_AUDIT(leaf);
}

void lf_rec_copy (Node_s *dst, int i, Node_s *src, int j)
{
FN;
	Hrec_s	rec = get_rec(src, j);
	store_rec(dst, rec.key, rec.val, i);
}

void lf_rec_move(Node_s *dst, int i, Node_s *src, int j)
{
FN;
	Hrec_s	rec = get_rec(src, j);
	store_rec(dst, rec.key, rec.val, i);
	lf_del_rec(src, j);
}

void br_twig_del (Node_s *branch, unint i)
{
FN;
	BR_AUDIT(branch);
	assert(i < branch->numrecs);

	--branch->numrecs;
	if (i < branch->numrecs) {
		memmove( &branch->twig[i], &branch->twig[i + 1],
			(branch->numrecs - i) * sizeof(Twig_s));
	}
	BR_AUDIT(branch);
}

void br_twig_move (Node_s *dst, int i, Node_s *src, int j)
{
FN;
	dst->twig[i] = dst->twig[j];
	br_twig_del(src, j);
}

Buf_s *node_new (Htree_s *t, u8 isleaf)
{
FN;
	Buf_s	*buf = buf_new(t->cache);

	buf->user = t;
	init_node(buf->d, isleaf, buf->blknum);
	return buf;
}

Buf_s *br_new (Htree_s *t)
{
FN;
	++t->stat.new_branches;
	return node_new(t, FALSE);
}

Buf_s *lf_new (Htree_s *t)
{
FN;
	++t->stat.new_leaves;
	return node_new(t, TRUE);
}

Buf_s *lf_split (Buf_s *buf)
{
FN;
	Node_s	*leaf     = buf->d;
	Htree_s	*t        = buf->user;
	Buf_s	*bsibling = lf_new(t);
	Node_s	*sibling  = bsibling->d;
	int	middle    = (leaf->numrecs + 1) / 2;
	int	i;

	LF_AUDIT(leaf);
	for (i = 0; middle < leaf->numrecs; i++) {
		lf_rec_move(sibling, i, leaf, middle);
	}
	LF_AUDIT(leaf);
	LF_AUDIT(sibling);
	++t->stat.split_leaf;
	return bsibling;
}

int lf_insert (Buf_s *buf, Key_t key, Lump_s val, int size)
{
FN;
	Node_s *leaf = buf->d;
	int i;

	LF_AUDIT(leaf);
	if (size > usable(leaf)) {
		lf_compact(leaf);
	}
	i = leaf_lt(leaf, key);
	store_rec(leaf, key, val, i);
	LF_AUDIT(leaf);
	buf_put_dirty( &buf);
	return 0;
}

bool is_full (Node_s *node, int size)
{
	if (node->isleaf) {
		return size > node->free;
	} else {
		return node->numrecs == NUM_TWIGS;
	}
}

bool is_sparse (Node_s *node)
{
	if (node->isleaf) {
		return node->free < LEAF_SPLIT;
	} else {
		return node->numrecs < BRANCH_SPLIT;
	}
}

Buf_s *grow (Htree_s *t, int size)
{
FN;
	Buf_s	*buf;
	Node_s	*node;
	Buf_s	*bparent;
	Node_s	*parent;

	if (!t->root) {
		buf = lf_new(t);
		t->root = buf->blknum;
		return buf;
	}
	buf = t_get(t, t->root);
	node = buf->d;
	if (!is_full(node, size)) return buf;
HERE;
	bparent = br_new(t);
	parent = bparent->d;
	parent->first = node->blknum;
	t->root = parent->blknum;
	buf_put( &buf);
	return bparent;
}

#if 0 /* xxx */
int br_reinsert(Op_s *op)
{
FN;
	Node_s *parent = op->parent->d;
	Node_s *node  = op->buf->d;
	Twig_s twig;

	twig.key = node->twig[node->numrecs - 1].key;
	??size = twig.key.size + SZ_U64 + LEAF_OVERHEAD;

	while (size > parent->free) {
		br_split(op);

		if (isLE(parent, twig.key, parent->numrecs - 1)) {
			buf_put_dirty( &op->sibling);
		} else {
			buf_put_dirty( &op->parent);
			op->parent = op->sibling;
			parent  = op->parent->d;
		}
		op->irec = leaf_le(parent, twig.key);
	}
	if (size > usable(parent)) {
		br_compact(parent);
	}
	twig.blknum = node->blknum;
	store_twig(parent, twig, op->irec);
	return 0;
}

int br_store(Op_s *op)
{
FN;
	Node_s *parent = op->parent->d;
	Node_s *node  = op->buf->d;
	int size;
	Twig_s twig;

	if (node->numrecs == 0) {
		/* We have a degenerate case */
		if (op->irec == parent->numrecs) {
			parent->last = node->last;
		} else {
			update_blknum(parent, node->last, op->irec);
		}
		buf_free( &op->buf);
		return 0;
	}
	twig.key = node->twig[node->numrecs - 1].key;
	size = sizeof(twig.key) + SZ_U64 + LEAF_OVERHEAD;

	while (size > parent->free) {
		br_split(op);

		if (isLE(parent, twig.key, parent->numrecs - 1)) {
			buf_put_dirty( &op->sibling);
		} else {
			buf_put_dirty( &op->parent);
			op->parent  = op->sibling;
			op->sibling = NULL;
			parent      = op->parent->d;
			op->irec    = leaf_le(parent, twig.key);
		}
	}
	if (size > usable(parent)) {
		br_compact(parent);
	}
	if (op->irec == parent->numrecs) {
		twig.blknum = parent->last;
		parent->last = node->last;
	} else {
		update_blknum(parent, node->last, op->irec);
		twig.blknum = node->blknum;
	}
	store_twig(parent, twig, op->irec);
	if (node->isleaf) {
		node->last = 0;
	} else {
		node->last = get_blknum(node, node->numrecs - 1);
		br_twig_del(node, node->numrecs - 1);
	}
	node->is_split = FALSE;
	buf_put_dirty( &op->buf);
	return 0;
}

int br_insert(Op_s *op)
{
FN;
	Node_s *parent;
	Node_s *node;
	Blknum_t blknum;

	op->parent = op->buf;
	op->buf = NULL;
	for(;;) {
		parent = op->parent->d;
		op->irec = leaf_le(parent, op->r.key);
		if (op->irec == parent->numrecs) {
			blknum = parent->last;
		} else {
			blknum = get_blknum(parent, op->irec);
		}
		op->buf = t_get(op->tree, blknum);
		node = op->buf->d;
		if (node->is_split) {
			if (br_store(op)) return FAILURE;
buf_dirty(op->parent);
			continue;
		}
		buf_put( &op->parent);
		if (node->isleaf) {
			lf_insert(op);
			return 0;
		}
		op->parent = op->buf;
		op->buf = NULL;
	}
}
#endif

void twig_insert (Node_s *branch, Key_t key, Blknum_t blknum, int irec)
{
PRd(key);
PRd(irec);
PRd(branch->numrecs);
	assert(!branch->isleaf);
	memmove( &branch->twig[irec + 1], &branch->twig[irec],
		(branch->numrecs - irec) * sizeof(Twig_s));
	branch->twig[irec].key = key;
	branch->twig[irec].blknum = blknum;
	++branch->numrecs;
	BR_AUDIT(branch);
}

void split_leaf (Htree_s *t, Node_s *parent, Node_s *leaf, int irec)
{
	Buf_s	*buf     = lf_new(t);
	Node_s	*sibling = buf->d;
	int	middle   = (leaf->numrecs + 1) / 2;
	Key_t	key;
	int	i;

	LF_AUDIT(leaf);
	for (i = 0; middle < leaf->numrecs; i++) {
		lf_rec_move(sibling, i, leaf, middle);
	}
	key = get_key(sibling, 0);
	twig_insert(parent, key, buf->blknum, irec);
	LF_AUDIT(leaf);
	LF_AUDIT(sibling);
	BR_AUDIT(parent);
	buf_put_dirty( &buf);
}

void split_branch (Htree_s *t, Node_s *parent, Node_s *branch, int irec)
{
	assert(branch->numrecs == NUM_TWIGS);
	BR_AUDIT(parent);
	BR_AUDIT(branch);

	Buf_s	*buf = br_new(t);
	Node_s	*sibling = buf->d;
	Twig_s	twig = branch->twig[MIDDLE_TWIG];

	sibling->first = twig.blknum;
	
	memmove(sibling->twig, &branch->twig[MIDDLE_TWIG + 1],
		sizeof(Twig_s) * (NUM_TWIGS - MIDDLE_TWIG - 1));
	sibling->numrecs = NUM_TWIGS - MIDDLE_TWIG - 1;
	branch->numrecs = MIDDLE_TWIG;

	twig_insert(parent, twig.key, sibling->blknum, irec);
	++t->stat.split_branch;
	BR_AUDIT(branch);
	BR_AUDIT(sibling);
	BR_AUDIT(parent);
	buf_put_dirty( &buf);
}

void br_split (Htree_s *t, Node_s *parent, Node_s *branch, int irec)
{
FN;
	Buf_s	*bsibling;
	Node_s	*sibling;
	int	middle = (branch->numrecs + 1) / 2;
	Twig_s	twig = branch->twig[middle];

	bsibling = br_new(t);
	sibling = bsibling->d;
	BR_AUDIT(branch);
	memmove(sibling->twig, &branch->twig[middle + 1], 
		(branch->numrecs - middle - 1) * sizeof(Twig_s));
	sibling->first = twig.blknum;
	sibling->numrecs = branch->numrecs - middle - 1;
	memmove( &parent->twig[irec + 1], &parent->twig[irec],
		(branch->numrecs - irec) * sizeof(Twig_s));
	parent->twig[irec].key = twig.key;
	parent->twig[irec].blknum = sibling->blknum;
	++parent->numrecs;
	BR_AUDIT(parent);
	BR_AUDIT(branch);
	BR_AUDIT(sibling);
	buf_put_dirty( &bsibling);
}

int t_insert(Htree_s *t, Key_t key, Lump_s val)
{
FN;
	int	size = val.size + LEAF_OVERHEAD;
	Node_s	*node;
	Node_s	*child;
	Buf_s	*buf;
	Buf_s	*bchild;
	int	irec;
	int	rc;

	buf = grow(t, size);
	node = buf->d;
	while (!node->isleaf) {
		irec = br_lt(node, key);
		bchild = t_get(t, node->twig[irec - 1].blknum);
		child = bchild->d;
		if (is_full(child, size)) {
			if (child->isleaf) {
				split_leaf(t, node, child, irec);
			} else {
				split_branch(t, node, child, irec);
			}
			buf_put_dirty( &bchild);
			buf_dirty(buf);
		} else {
			buf_put( &buf);
			buf = bchild;
			node = child;
		}	
	}
	rc = lf_insert(buf, key, val, size);
	cache_balanced(t->cache);
	++t->stat.insert;
	return rc;
}

#if 0
int lf_find(Buf_s *bleaf, Key_t key, Lump_s *val)
{
FN;
	Buf_s *right;
	Node_s *node = bleaf->d;
	Lump_s v;
	int i;

	for (;;) {
		i = leaf_eq(node, key);
		if (i == -1) {
			return HT_ERR_NOT_FOUND;
		}
		if (i == node->numrecs) {
			if (node->is_split) {
				right = t_get(bleaf->user, node->last);
				buf_put( &bleaf);
				bleaf = right;
				node = bleaf->d;
			} else {
				return HT_ERR_NOT_FOUND;
			}
		} else {
			v = get_val(node, i);
			*val = duplump(v);
			buf_put( &bleaf);
			return 0;
		}
	}
}

int br_find(Buf_s *bparent, Key_t key, Lump_s *val)
{
FN;
	Node_s *parent = bparent->d;
	Buf_s *buf;
	Node_s *node;
	Blknum_t blknum;
	int x;

	for(;;) {
		x = leaf_le(parent, key);
		if (x == parent->numrecs) {
			blknum = parent->last;
		} else {
			blknum = get_blknum(parent, x);
		}
		buf = t_get(bparent->user, blknum);
		node = buf->d;
		if (node->isleaf) {
			buf_put_dirty( &bparent);
			lf_find(buf, key, val);
			return 0;
		}
		buf_put_dirty( &bparent);
		bparent = buf;
		parent = bparent->d;
	}
}

int t_find(Htree_s *t, Key_t key, Lump_s *val)
{
FN;
	Buf_s *buf;
	Node_s *node;
	int rc;

	if (!t->root) {
		return HT_ERR_NOT_FOUND;
	}
	buf = t_get(t, t->root);
	node = buf->d;
	if (node->isleaf) {
		rc = lf_find(buf, key, val);
	} else {
		rc = br_find(buf, key, val);
	}
	cache_balanced(t->cache);
	if (!rc) ++t->stat.find;
	return rc;
}

bool join(Op_s *op)
{
FN;
	Node_s *parent  = op->parent->d;
	Node_s *node    = op->buf->d;
	Node_s *sibling = op->sibling->d;
	int i;

	if (sibling->is_split) {
		/* TODO(taysom): Need to think about what it means
		 * join a sibling that is split. May want to schedule
		 * it or something else. Could lead to problems (but should
		 * be rare).
		 */
		return FALSE;
	}
	if (node->isleaf) {
		if (inuse(node) > sibling->free) {
			return FALSE;
		}
		lf_compact(sibling);
		for (i = 0; node->numrecs; i++) {
			lf_rec_move(sibling, i, node, 0);
		}
	} else {
		Twig_s twig;
		int used = inuse(node);

		twig.key = parent->twig[op->irec].key;
		used += sizeof(twig.key) + SZ_U64 + SZ_BRANCH_OVERHEAD;
		if (used > sibling->free) {
			return FALSE;
		}
		br_compact(sibling);
		for (i = 0; node->numrecs; i++) {
			br_twig_move(sibling, i, node, 0);
		}
		twig.blknum = node->last;
		store_twig(sibling, twig, i);
	}
	br_twig_del(parent, op->irec);
	buf_free( &op->buf);
	op->buf = op->sibling;
	op->sibling = NULL;
	buf_dirty(op->parent);
	buf_dirty(op->buf);
	++op->tree->stat.join;
	return TRUE;
}

int lf_rebalance(Op_s *op)
{
FN;
	Node_s *parent  = op->parent->d;
	Node_s *node    = op->buf->d;
	Node_s *sibling = op->sibling->d;
	int i;

	lf_compact(node);
	for (i = 0; ;i++) {
		lf_rec_move(node, node->numrecs, sibling, 0);
		if (node->free <= sibling->free) break;
	}
	br_twig_del(parent, op->irec);
	br_reinsert(op);
	return 0;
}

/* br_rebalance - rebalances children that are branches
 *
 * Get key from parent and blknum from node.last and append to node.
 * Get a record from sibling
 * If done - store key in parent and last in node
 *  else - store in node
 */
int br_rebalance(Op_s *op)
{
FN;
	Node_s *parent  = op->parent->d;
	Node_s *node    = op->bug->d;
	Node_s *sibling = op->sibling->d;
	u64 twigblknum;
	Twig_s twig;

	br_compact(node);
	twig = parent->twig[op->irec];
	twigblknum = twig.blknum;
	twig.blknum = node->last;
	store_twig(node, twig, node->numrecs);
	br_twig_del(parent, op->irec);
	for (;;) {
		twig = sibling->twig[0];
		if (node->free <= sibling->free) break;
		store_twig(node, twig, node->numrecs);
		br_twig_del(sibling, 0);
	}
	node->last = twig.blknum;
	twig.blknum = twigblknum;
	store_twig(parent, twig, op->irec);
	br_twig_del(sibling, 0);
	return 0;
}

int rebalance(Op_s *op)
{
FN;
	Node_s *parent = op->parent->d;
	Node_s *node   = op->buf->d;
	Node_s *sibling;
	Blknum_t blknum;
	int y;

	if (op->irec == parent->numrecs) {
		/* No right sibling */
		return 0;
	}
	y = op->irec + 1;
	if (y == parent->numrecs) {
		blknum = parent->last;
	} else {
		blknum = get_blknum(parent, y);
	}
	op->sibling = t_get(op->tree, blknum);
	sibling = op->sibling->d;

	if (join(op)) return 0;

	if (sibling->numrecs == 0) {
		// TODO(taysom) may want to do some thing with sibling?
		buf_put( &op->sibling);
		return 0;
	}
	assert(sibling->numrecs > 0);
	if (node->isleaf) {
		lf_rebalance(op);
	} else {
		br_rebalance(op);
	}
	buf_dirty(op->parent);
	buf_dirty(op->buf);
	buf_put_dirty( &op->sibling);
	return 0;
}

int br_delete(Op_s *op)
{
	Node_s *parent;
	Node_s *node;
	Blknum_t blknum;

	op->parent = op->buf;
	op->buf = NULL;
	for(;;) {
FN;
		parent = op->parent->d;
		op->irec = leaf_le(parent, op->r.key);
		blknum = parent->twig[op->irec - 1].blknum;
		op->buf = t_get(op->tree, blknum);
		node = op->buf->d;
		if (node->free > REBALANCE) {
			if (rebalance(op)) return FAILURE;
			node = op->buf->d;
		}
		buf_put( &op->parent);
		if (node->isleaf) {
			lf_delete(op);
			return 0;
		}
		op->parent = op->buf;
		op->buf = NULL;
	}
}

void join_leaf (Htree_s *t, Node_s *node, Buf_s *bchild, int irec)
{
	Node_s	*child = bchild->d;
	Buf_s	*bsibling;
	Node_s	*sibling;

	if (irec == node->numrecs) return;	/* No right sibling */

	bsibling = t_get(t, node->twig[i].blknum);
	sibling = bsibling->d;
	if (inuse(sibling) < child->free) {
		lf_compact(child);
		for (i = 0; i)
		lf_rec_move(node,); 
	} else {
		/* Rebalance */
	}
}

bool pr_key_leaf (Node_s *node, unint i)
{
	unint size;
	unint x;
	u8 *start;
	Key_t key;

	if (i >= node->numrecs) {
		printf("key index >= numrecs %ld >= %d\n",
			i, node->numrecs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= BLOCK_SIZE) {
		printf("key offset >= BLOCK_SIZE %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	UNPACK(start, key);
	size = sizeof(Key_t);
	if (x + size >= BLOCK_SIZE) {
		printf("key size at %ld offset %ld is too big %ld\n",
			i, x, size);
		return FALSE;
	}
	printf(" %4ld, %4ld:", x, size);
	pr_key(key);
	putchar('\n');
	return TRUE;
}

bool pr_val (Node_s *node, unint i)
{
	unint key_size;
	unint size;
	unint x;
	u8 *start;

	if (i >= node->numrecs) {
		printf("key index >= numrecs %ld >= %d\n",
			i, node->numrecs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= BLOCK_SIZE) {
		printf("key offset >= BLOCK_SIZE %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	if (x + key_size >= BLOCK_SIZE) {
		printf("key size at %ld offset %ld is too big %ld\n",
			i, x, key_size);
		return FALSE;
	}
	start += key_size;
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= BLOCK_SIZE) {
		printf("val size at %ld offset %ld is too big %ld\n",
			i, x, size);
		return FALSE;
	}
	printf(" %4ld, %4ld:", x, size);
	pr_lump(lumpmk(size, start));
	putchar('\n');
	return TRUE;
}

bool pr_record (Node_s *node, unint i)
{
	unint size;
	unint x;
	u8 *start;

	if (i >= node->numrecs) {
		printf("key index >= numrecs %ld >= %d\n",
			i, node->numrecs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= BLOCK_SIZE) {
		printf("key offset >= BLOCK_SIZE %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= BLOCK_SIZE) {
		printf("key size at %ld offset %ld is too big %ld\n",
			i, x, size);
		return FALSE;
	}
	printf(" %4ld, %4ld:", x, size);
	pr_lump(lumpmk(size, start));
	start += size;
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= BLOCK_SIZE) {
		printf("val size at %ld offset %ld is too big %ld\n",
			i, x, size);
		return FALSE;
	}
	printf(" : %4ld:", size);
	pr_lump(lumpmk(size, start));
	putchar('\n');
	return TRUE;
}

void pr_leaf(Node_s *node)
{
	unint i;

	pr_head(node, 0);
	for (i = 0; i < node->numrecs; i++) {
		pr_record(node, i);
	}
}

bool pr_twig (Node_s *node, unint i)
{
	unint size;
	unint x;
	u8 *start;
	Blknum_t blknum;

	if (i >= node->numrecs) {
		printf("key index >= numrecs %ld >= %d\n",
			i, node->numrecs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= BLOCK_SIZE) {
		printf("key offset >= BLOCK_SIZE %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= BLOCK_SIZE) {
		printf("key size at %ld offset %ld is too big %ld\n",
					i, x, size);
		return FALSE;
	}
	printf(" %4ld, %4ld:", x, size);
	for (i = 0; i < size; i++) {
		if (isprint(start[i])) {
			printf("%c", start[i]);
		} else {
			putchar('.');
		}
	}
	start += size;

	UNPACK(start, blknum);
	printf(" : %lld", blknum);
	putchar('\n');
	return TRUE;
}

void pr_branch(Node_s *node)
{
	unint i;

	pr_head(node, 0);
	printf(" first: %llu\n", (u64)node->first);
	for (i = 0; i < node->numrecs; i++) {
		pr_twig(node, i);
	}
}

void pr_node(Node_s *node)
{
	if (node->isleaf) {
		pr_leaf(node);
	} else {
		pr_branch(node);
	}
}

static unint Recnum;

static void pr_all_nodes(Htree_s *t, Blknum_t blknum);

static void pr_all_leaves(Buf_s *buf)
{
	Node_s *node = buf->d;
	Hrec_s rec;
	unint i;

	for (i = 0; i < node->numrecs; i++) {
		rec = get_rec(node, i);
		printf("%4ld. ", Recnum++);
		key_dump(rec.key);
		printf("\t");
		lump_dump(rec.val);
		printf("\n");
	}
}

static void pr_all_branches(Buf_s *buf)
{
	Node_s *node = buf->d;
	Blknum_t blknum;
	unint i;

	pr_all_nodes(buf->user, node->first);
	for (i = 0; i < node->numrecs; i++) {
		blknum = get_blknum(node, i);
		pr_all_nodes(buf->user, blknum);
	}
}

static void pr_all_nodes(Htree_s *t, Blknum_t blknum)
{
	Buf_s *buf;
	Node_s *node;

	if (!blknum) return;
	buf = t_get(t, blknum);
	node = buf->d;
	if (node->isleaf) {
		pr_all_leaves(buf);
	} else {
		pr_all_branches(buf);
	}
	buf_put( &buf);
}

void pr_all_records(Htree_s *t)
{
	printf("=====All Records in Order=======\n");
	Recnum = 1;
	pr_all_nodes(t, t->root);
	cache_balanced(t->cache);
}

static int node_map(Htree_s *t, Blknum_t blknum, Apply_s apply);

static int lf_map(Buf_s *buf, Apply_s apply)
{
	Node_s *node = buf->d;
	unint i;
	Hrec_s rec;
	int rc;

	for (i = 0; i < node->numrecs; i++) {
		rec = get_rec(node, i);
		rc = apply.func(rec, apply.tree, apply.user);
		if (rc) return rc;
	}
	return 0;
}

static int br_map(Buf_s *buf, Apply_s apply)
{
	Node_s *node = buf->d;
	Blknum_t blknum;
	unint i;
	int rc;

	rc = node_map(buf->user, node->first, apply);
	if (rc) return rc;
	for (i = 0; i < node->numrecs; i++) {
		blknum = get_blknum(node, i);
		rc = node_map(buf->user, blknum, apply);
		if (rc) return rc;
	}
	return 0;
}

static int node_map(Htree_s *t, Blknum_t blknum, Apply_s apply)
{
	Buf_s *buf;
	Node_s *node;
	int rc = 0;

	if (!blknum) return rc;
	buf = t_get(t, blknum);
	node = buf->d;
	if (node->isleaf) {
		rc = lf_map(buf, apply);
	} else {
		rc = br_map(buf, apply);
	}
	buf_put( &buf);
	return rc;
}

int t_map(Htree_s *t, Apply_f func, void *sys, void *user)
{
	Apply_s apply = mk_apply(func, t, sys, user);
	int rc = node_map(t, t->root, apply);
	cache_balanced(t->cache);
	return rc;
}

static int map_rec_audit (Hrec_s rec, Htree_s *t, void *user)
{
	Key_t *oldkey = user;

	if (rec.key < *oldkey) {
		t_dump(t);
		pr_key(rec.key);
		printf(" <= ");
		pr_key(*oldkey);
		printf("  ");
		fatal("keys out of order");
		return FAILURE;
	}
	*oldkey = rec.key;
	return 0;
}

static int node_audit(Htree_s *t, Blknum_t blknum, Audit_s *audit, int depth);

static int leaf_audit(Buf_s *buf, Audit_s *audit, int depth)
{
	Node_s *node = buf->d;

	++audit->leaves;
#if 0
	unint i;
	Hrec_s rec;
	int rc;
	for (i = 0; i < node->numrecs; i++) {
		rec = get_rec(node, i);
		rc = apply.func(rec, apply.user);
		if (rc) return rc;
	}
#endif
	audit->records += node->numrecs;
	if (audit->max_depth) {
		if (depth != audit->max_depth) {
			t_dump(buf->user);
			fatal("Depth is wrong at blknum %lld %d != %d\n",
				buf->blknum, depth, audit->max_depth);
		}
	} else {
		audit->max_depth = depth;
	}
	return 0;
}

static int branch_audit(Buf_s *buf, Audit_s *audit, int depth)
{
	Node_s *node = buf->d;
	Blknum_t blknum;
	unint i;
	int rc;

	++audit->branches;
	rc = node_audit(buf->user, node->first, audit, depth);
	if (rc) return rc;
	for (i = 0; i < node->numrecs; i++) {
		blknum = get_blknum(node, i);
		rc = node_audit(buf->user, blknum, audit, depth);
		if (rc) return rc;
	}
	return 0;
}

static int node_audit(Htree_s *t, Blknum_t blknum, Audit_s *audit, int depth)
{
	Buf_s *buf;
	Node_s *node;
	int rc = 0;

	if (!blknum) return rc;
	buf = t_get(t, blknum);
	node = buf->d;
	if (node->isleaf) {
		rc = leaf_audit(buf, audit, depth+1);
	} else {
		rc = branch_audit(buf, audit, depth+1);
	}
	buf_put( &buf);
	return rc;
}


int t_audit (Htree_s *t, Audit_s *audit)
{
	Key_t oldkey = 0;
	int rc;

	zero(*audit); 
	rc = t_map(t, map_rec_audit, NULL, &oldkey);
	if (rc) {
		printf("AUDIT FAILED %d\n", rc);
		return rc;
	}
	rc = node_audit(t, t->root, audit, 0);
	return rc;
}
#endif

int lf_delete (Buf_s *buf, Key_t key)
{
	Node_s	*node = buf->d;
	int	x;

	for (;;) {
FN;
		x = leaf_eq(node, key);
		if (x == -1) {
			return HT_ERR_NOT_FOUND;
		}
		if (x == node->numrecs) {
			return HT_ERR_NOT_FOUND;
		} else {
			delete_rec(node, x);
			buf_put_dirty( &buf);
			return 0;
		}
	}
}

int t_delete(Htree_s *t, Key_t key)
{
FN;
	Buf_s	*buf;
	Node_s	*node;
	Buf_s	*bchild;
	Node_s	*child;
	int	irec;
	int	rc;

	if (!t->root) return HT_ERR_NOT_FOUND;
	buf = t_get(t, t->root);
	node = buf->d;
	while (node && node->numrecs == 0) {
		if (node->isleaf) {
			t->root = 0;
		} else {
			t->root = node->first;
		}
		buf_free( &buf);
		if (!t->root) return HT_ERR_NOT_FOUND;
		buf = t_get(t, t->root);
		node = buf->d;
	}
	while (!node->isleaf) {
		irec = br_lt(node, key);
		bchild = t_get(t, node->twig[irec - 1].blknum);
		child = bchild->d;
		if (is_sparse(child)) {
			if (child->isleaf) {
				join_leaf(t, node, bchild, irec);
			} else {
				join_branch(t, node, bchild, irec);
			}
			buf_free( &bchild);
		} else {
			buf_free( &buf);
			buf = bchild;
			node = child;
		}	
	}
	rc = lf_delete(buf, key);
	cache_balanced(t->cache);
	if (!rc) ++t->stat.delete;
	return rc;
}

void t_dump(Htree_s *t)
{
	int is_on = fdebug_is_on();

	printf("\n************************t_dump**************************\n");
	if (is_on) fdebugoff();
	node_dump(t, t->root, 0);
	if (is_on) fdebugon();
	//cache_balanced(t->cache);
}

Twig_s CheckTwig[NUM_TWIGS];

Htree_s *t_new(char *file, int num_bufs)
{
FN;
	Htree_s *t;

	t = ezalloc(sizeof(*t));
	t->cache = cache_new(file, num_bufs, BLOCK_SIZE);
PRd(BLOCK_SIZE);
PRd(NUM_TWIGS);
PRd(sizeof(Twig_s));
PRd(sizeof(Node_s));
PRd(sizeof(CheckTwig));
	return t;
}

#if 0
Htree_s *t_new(char *file, int num_bufs) {
	warn("Not Implmented");
	return 0;
}

void t_dump  (Htree_s *t) {
	warn("Not Implmented");
}

int  t_insert(Htree_s *t, Key_t key, Lump_s val) {
	warn("Not Implmented");
	return 0;
}

int  t_delete(Htree_s *t, Key_t key) {
	warn("Not Implmented");
	return 0;
}
#endif

int  t_find  (Htree_s *t, Key_t key, Lump_s *val) {
	warn("Not Implmented");
	return 0;
}

int  t_map   (Htree_s *t, Apply_f func, void *sys, void *user) {
	warn("Not Implmented");
	return 0;
}

int t_audit (Htree_s *t, Audit_s *audit) {
	warn("Not Implmented");
	return 0;
}
