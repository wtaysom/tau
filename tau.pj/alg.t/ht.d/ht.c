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
	SZ_BUF  = 128,
	SZ_U16  = sizeof(u16),
	SZ_U64  = sizeof(u64),
	SZ_HEAD = sizeof(Node_s),
	SZ_LEAF_OVERHEAD   = 3 * SZ_U16,
	SZ_BRANCH_OVERHEAD = 2 * SZ_U16,
	REBALANCE = 2 * (SZ_BUF - SZ_HEAD) / 3 };

typedef struct Err_s {
	const char *fn;
	const char *label;
	int line;
	int err;
} Err_s;

#define SET_ERR(_e, _rc) (((_e).where = WHERE), ((_e).rc = (_rc)))

struct Htree_s {
	Cache_s *cache;
	u64 root;
	Stat_s stat;
};

typedef struct Apply_s {
	Apply_f func;
	Htree_s *tree;
	void *sys;
	void *user;
} Apply_s;

typedef struct Hrec_s {
	Key_t	key;
	Lump_s	val;
} Hrec_s;

typedef struct Op_s {
	Htree_s *tree;
	Buf_s *parent;
	Buf_s *buf;
	Buf_s *sibling;
	int irec;  /* Record index */
	Hrec_s r;
	Err_s err;
} Op_s;

#define PRop(_op) pr_op(FN_ARG, # _op, _op)
#define ERR(_err) t_err(FN_ARG, # _err, op, _err)

void lump_dump(Lump_s a);
void lf_dump(Buf_s *buf, int indent);
void br_dump(Buf_s *buf, int indent);
void node_dump(Htree_s *t, Blknum_t blknum, int indent);
void pr_leaf(Node_s *node);
void pr_branch(Node_s *node);
void pr_node(Node_s *node);

bool Dump_buf = FALSE;

static inline Apply_s mk_apply(Apply_f func, Htree_s *t, void *sys, void *user)
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
		"  num bufs =%8d\n"
		"  gets     =%8lld\n"
		"  puts     =%8lld\n"
		"  hits     =%8lld\n"
		"  miss     =%8lld\n"
		"hit ratio  =%8g%%\n",
		cs.numbufs,
		cs.gets, cs.puts, cs.hits, cs.miss,
		(double)(cs.hits) / (cs.hits + cs.miss) * 100.);
}

int t_err(
	const char *fn,
	unsigned line,
	const char *label,
	Op_s *op,
	int err)
{
	op->err.fn = fn;
	op->err.line = line;
	op->err.label = label;
	op->err.err   = err;
	printf("ERROR: %s<%d> %s: %d\n",
		fn, line, label, err);

	printf("t_err\n");
	t_dump(op->tree);
	cache_err(op->tree);
exit(1);
	return err;
}

Buf_s *t_get (Htree_s *t, Blknum_t blknum)
{
	Buf_s *buf = buf_get(t->cache, blknum);
	buf->user = t;
	return buf;
}

bool pr_op(const char *fn, unsigned line, const char *label, Op_s *op)
{
	return print(fn, line,
		"%s: tree=%p parent=%p buf=%p sibling=%p"
		" irec=%d key=%.*s val=%.*s err=%d",
		label, op->tree, op->parent, op->buf, op->sibling,
		op->irec, op->r.key, op->val, op->err.err);
}

Stat_s t_get_stats (Htree_s *t)
{ return t->stat; }
CacheStat_s t_cache_stats (Htree_s *t)
{ return cache_stats(t->cache); }

int usable(Node_s *node)
{
FN;
	return node->end - SZ_HEAD - SZ_U16 * node->num_recs;
}

int inuse(Node_s *node)
{
	return SZ_BUF - SZ_HEAD - node->free;
}

void pr_indent(int indent)
{
	int i;

	for (i = 0; i < indent; i++) {
		printf("  ");
	}
}

void pr_node(Node_s *node, int indent)
{
	pr_indent(indent);
	printf("%s "
		"%s"
		"%lld num_recs %d "
		"free %d "
		"end %d "
		"last %lld\n",
		node->kind == LEAF ? "LEAF" : "BRANCH",
		node->is_split ? "*split* " : "",
		node->blknum,
		node->num_recs,
		node->free,
		node->end,
		node->last);
}

void pr_buf(Buf_s *buf, int indent)
{
	enum { BYTES_PER_ROW = 16 };
	unint i;
	unint j;
	u8 *d = buf->d;
	char *c = buf->d;

	pr_indent(indent);
	printf("blknum=%lld\n", buf->blknum);
	for (i = 0; i < SZ_BUF / BYTES_PER_ROW; i++) {
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
	int i;

	for (i = 0; i < n; i++) {
		if (isprint(a[i])) {
			putchar(a[i]);
		} else {
			putchar('.');
		}
	}

}

static void pr_lump (Lump_s a)
{
	enum { MAX_PRINT = 32 };
	int size;

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
	pr_lump(rec.key);
	printf(" = ");
	pr_lump(rec.val);
}

void init_node(Node_s *node, u8 kind, Blknum_t blknum)
{
FN;
	node->kind     = kind;
	node->num_recs = 0;
	node->is_split = FALSE;
	node->free     = SZ_BUF - SZ_HEAD;
	node->end      = SZ_BUF;
	node->blknum   = blknum;
	node->last     = 0;
}

Key_t get_key (Node_s *node, unint i)
{
	Key_t key;
	unint x;
	u8 *start;

	assert(i < node->num_recs);
	x = node->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)node)[x];
	UNPACK(start, key);
	return key;
}

Lump_s get_val (Node_s *node, unint i)
{
	Lump_s val;
	unint x;
	u8 *start;

	assert(node->kind == LEAF);
	assert(i < node->num_recs);
	x = node->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)node)[x];
	start += sizeof(Key_t);
	val.size = *start++;
	val.size |= (*start++) << 8;
	val.d = start;
	return val;
}

Hrec_s get_rec (Node_s *node, unint i)
{
	Hrec_s rec;
	unint x;
	u8 *start;

if (node->kind != LEAF)
{
	pr_node(node, 0);
}
	assert(node->kind == LEAF);
	assert(i < node->num_recs);
	x = node->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)node)[x];
	rec.key.size = *start++;
	rec.key.size |= (*start++) << 8;
	rec.key.d = start;
	start += rec.key.size;
	rec.val.size = *start++;
	rec.val.size |= (*start++) << 8;
	rec.val.d = start;
	return rec;
}

u64 get_blknum(Node_s *node, unint a)
{
FN;
	Blknum_t blknum;
	unint x;
	u8 *start;
	unint key_size;

	assert(node->kind == BRANCH);
	assert(a < node->num_recs);
	x = node->rec[a];
	assert(x < SZ_BUF);
	start = &((u8 *)node)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	start += key_size;
	UNPACK(start, blknum);
	return blknum;
}

Twig_s get_twig(Node_s *node, unint a)
{
FN;
	Twig_s twig;
	u8 *start;
	unint x;

	assert(node->kind == BRANCH);
	assert(a < node->num_recs);
	x = node->rec[a];
	assert(x < SZ_BUF);
	start = &((u8 *)node)[x];
	twig.key.size = *start++;
	twig.key.size |= (*start++) << 8;
	twig.key.d = start;
	start += twig.key.size;
	UNPACK(start, twig.blknum);
	return twig;
}

void lump_dump(Lump_s a)
{
FN;
#if 0
	int i;

	for (i = 0; i < a.size; i++) {
		putchar(a.d[i]);
	}
#else
	printf("%.*s", a.size, a.d);
#endif
}

void rec_dump (Hrec_s rec)
{
	lump_dump(rec.key);
	printf(" = ");
	lump_dump(rec.val);
	printf("\n");
}

void lf_dump(Buf_s *buf, int indent)
{
	Node_s *node = buf->d;
	Hrec_s rec;
	unint i;

	if (Dump_buf) {
		pr_buf(buf, indent);
	}
	pr_node(node, indent);
	for (i = 0; i < node->num_recs; i++) {
		rec = get_rec(node, i);
		pr_indent(indent);
		printf("%ld. ", i);
		rec_dump(rec);
	}
	if (node->is_split) {
		pr_indent(indent);
		printf("Leaf Split:\n");
		node_dump(buf->user, node->last, indent);
	}
}

void br_dump(Buf_s *buf, int indent)
{
	Node_s *node = buf->d;
	Twig_s twig;
	unint i;

	pr_node(node, indent);
	for (i = 0; i < node->num_recs; i++) {
		twig = get_twig(node, i);
		pr_indent(indent);
		printf("%ld. ", i);
		lump_dump(twig.key);
		printf(" = %lld\n", twig.blknum);
		node_dump(buf->user, twig.blknum, indent + 1);
	}
	if (node->is_split) {
		pr_indent(indent);
		printf("Branch Split:\n");
		node_dump(buf->user, node->last, indent);
	} else {
		pr_indent(indent);
		printf("%ld. <last> = %lld\n", i, node->last);
		node_dump(buf->user, node->last, indent + 1);
	}
}

void node_dump(Htree_s *t, Blknum_t blknum, int indent)
{
	Buf_s *buf;
	Node_s *node;

	if (!blknum) return;
	buf = t_get(t, blknum);
	node = buf->d;
	switch (node->kind) {
	case LEAF:
		lf_dump(buf, indent);
		break;
	case BRANCH:
		br_dump(buf, indent + 1);
		break;
	default:
		printf("unknown kind %d\n", node->kind);
		break;
	}
	buf_put(&buf);
}

#if 0
bool isGE(Node_s *node, Key_t key, unint i)
{
FN;
	Key_t target;

	target = get_key(node, i);
	return target >= key;
}
#endif

bool isLE(Node_s *node, Key_t key, unint i)
{
	Key_t target;

	target = get_key(node, i);
	return target <= key;
}

int isEQ(Node_s *node, Key_t key, unint i)
{
	Key_t target;

	target = get_key(node, i);
	if (target < key) return -1;
	if (target == key) return 0;
	return 1;
}

void store_key(Node_s *node, Key_t key)
{
FN;
	u8 *start;

	assert(sizeof(Key_t) <= node->free);
	assert(sizeof(Key_t) <= node->end);

	node->end -= sizeof(Key_t);
	node->free -= sizeof(Key_t);
	start = &((u8 *)node)[node->end];
	PACK(start, key);
}

void store_lump(Node_s *node, Lump_s lump)
{
FN;
	int total = lump.size + SZ_U16;
	u8 *start;

	assert(total <= node->free);
	assert(total <= node->end);
	assert(lump.size < MAX_U16);

	node->end -= total;
	node->free -= total;
	start = &((u8 *)node)[node->end];
	*start++ = lump.size & MASK_U8;
	*start++ = (lump.size >> BITS_U8) & MASK_U8;
	memmove(start, lump.d, lump.size);
}

void store_blknum(Node_s *node, Blknum_t blknum)
{
FN;
	int total = SZ_U64;
	u8 *start;

	assert(total <= node->free);
	assert(total <= node->end);

	node->end -= total;
	node->free -= total;
	start = &((u8 *)node)[node->end];
	PACK(start, blknum);
}

void update_blknum(Node_s *node, Blknum_t blknum, unint irec)
{
FN;
	unint x;
	u8 *start;
	unint key_size;

	assert(node->kind == BRANCH);
if (irec >= node->num_recs)
{
	PRd(irec);
	PRd(node->num_recs);
	PRd(node->blknum);
}
	assert(irec < node->num_recs);
	x = node->rec[irec];
	assert(x < SZ_BUF);
	start = &((u8 *)node)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	start += key_size;
	PACK(start, blknum);
}

/* XXX: Don't like this name */
void store_end(Node_s *node, unint i)
{
FN;
	assert(i <= node->num_recs);
	if (i < node->num_recs) {
		memmove(&node->rec[i+1], &node->rec[i],
			(node->num_recs - i) * SZ_U16);
	}
	++node->num_recs;
	node->rec[i] = node->end;
	node->free -= SZ_U16;
}

void rec_copy(Node_s *dst, int a, Node_s *src, int b)
{
	Hrec_s rec = get_rec(src, b);

	store_lump(dst, rec.val);
	store_key(dst, rec.key);
	store_end(dst, a);
}

void twig_copy(Node_s *dst, int a, Node_s *src, int b)
{
	Twig_s twig;

	twig = get_twig(src, b);

	store_blknum(dst, twig.blknum);
	store_key(dst, twig.key);
	store_end(dst, a);
}

void lf_compact(Node_s *leaf)
{
FN;
	u8 b[SZ_BUF];
	Node_s *h = (Node_s *)b;
	int i;

	if (usable(leaf) == leaf->free) {
		return;
	}
	init_node(h, LEAF, leaf->blknum);
	h->is_split  = leaf->is_split;
	h->last      = leaf->last;
	for (i = 0; i < leaf->num_recs; i++) {
		rec_copy(h, i, leaf, i);
	}
	memmove(leaf, h, SZ_BUF);
}

void br_compact(Node_s *branch)
{
FN;
	u8 b[SZ_BUF];
	Node_s *h = (Node_s *)b;
	int i;

	if (usable(branch) == branch->free) {
		return;
	}
	init_node(h, BRANCH, branch->blknum);
	h->is_split  = branch->is_split;
	h->last      = branch->last;
	for (i = 0; i < branch->num_recs; i++) {
		twig_copy(h, i, branch, i);
	}
	memmove(branch, h, SZ_BUF);
}

void store_rec(Node_s *node, Hrec_s rec, unint i)
{
FN;
	lf_compact(node);
	store_lump(node, rec.val);
	store_key(node, rec.key);
	store_end(node, i);
}

void store_twig(Node_s *node, Twig_s twig, unint i)
{
FN;
	br_compact(node);
	store_blknum(node, twig.blknum);
	store_key(node, twig.key);
	store_end(node, i);
}

void delete_rec(Node_s *node, unint i)
{
FN;
	Hrec_s rec;

	assert(i < node->num_recs);
	assert(node->kind == LEAF);
	rec = get_rec(node, i);
	node->free += rec.key.size + rec.val.size + SZ_LEAF_OVERHEAD;
	--node->num_recs;
	if (i < node->num_recs) {
		memmove(&node->rec[i], &node->rec[i+1],
			(node->num_recs - i) * SZ_U16);
	}
}

void lf_audit(const char *fn, unsigned line, Node_s *node)
{
FN;
	Hrec_s rec;
	int free = SZ_BUF - SZ_HEAD;
	int i;

	assert(node->kind == LEAF);
	for (i = 0; i < node->num_recs; i++) {
		rec = get_rec(node, i);
		free -= rec.key.size + rec.val.size + 3 * SZ_U16;
	}
	if (free != node->free) {
		printf("%s<%d> free:%d != %d:node->free\n",
			fn, line, free, node->free);
		exit(2);
	}
}

void br_audit(const char *fn, unsigned line, Node_s *node)
{
FN;
	int i;
	int free = SZ_BUF - SZ_HEAD;

	assert(node->kind == BRANCH);
	for (i = 0; i < node->num_recs; i++) {
		Key_t key;

		key = get_key(node, i);
		free -= key.size + SZ_U64 + 2 * SZ_U16;
	}
	if (free != node->free) {
		printf("%s<%d> free:%d != %d:node->free\n",
			fn, line, free, node->free);
		exit(2);
	}
}

int leaf_le(Node_s *node, Key_t key)
{
FN;
	int x;
	int left;
	int right;

	left = 0;
	right = node->num_recs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (isLE(node, key, x)) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return left;
}

#if 0
int leaf_ge(Node_s *node, Key_t key)
{
FN;
	int x;
	int left;
	int right;

	left = 0;
	right = node->num_recs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (isGE(node, key, x)) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	return right;
}
#endif

int leaf_eq(Node_s *node, Key_t key)
{
FN;
	int x;
	int left;
	int right;
	int eq;

	left = 0;
	right = node->num_recs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		eq = isEQ(node, key, x);
		if (eq == 0) {
			return x;
		} else if (eq > 0) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	if (left == node->num_recs) return left;
	return -1;
}

void lf_del_rec(Node_s *node, unint i)
{
FN;
	Hrec_s rec;

	LF_AUDIT(node);
	assert(i < node->num_recs);
	rec = get_rec(node, i);
	node->free += rec.key.size + rec.val.size + 3 * SZ_U16;

	--node->num_recs;
	if (i < node->num_recs) {
		memmove(&node->rec[i], &node->rec[i+1],
			(node->num_recs - i) * SZ_U16);
	}
	LF_AUDIT(node);
}

void lf_rec_copy(Node_s *dst, int i, Node_s *src, int j)
{
FN;
	Hrec_s rec = get_rec(src, j);
	store_rec(dst, rec, i);
}

void lf_rec_move(Node_s *dst, int i, Node_s *src, int j)
{
FN;
	Hrec_s rec = get_rec(src, j);
	store_rec(dst, rec, i);
	lf_del_rec(src, j);
}

void br_twig_del(Node_s *node, unint i)
{
FN;
	Key_t key;

	BR_AUDIT(node);
	assert(i < node->num_recs);
	key = get_key(node, i);
	node->free += key.size + SZ_U64 + 2 * SZ_U16;

	--node->num_recs;
	if (i < node->num_recs) {
		memmove(&node->rec[i], &node->rec[i+1],
			(node->num_recs - i) * SZ_U16);
	}
	BR_AUDIT(node);
}

void br_twig_copy(Node_s *dst, int i, Node_s *src, int j)
{
FN;
	Twig_s twig;

	twig = get_twig(src, j);

	store_twig(dst, twig, i);
}

void br_twig_move(Node_s *dst, int i, Node_s *src, int j)
{
FN;
	br_twig_copy(dst, i, src, j);
	br_twig_del(src, j);
}

Buf_s *node_new(Htree_s *t, u8 kind)
{
FN;
	Buf_s *buf = buf_new(t->cache);
	buf->user = t;
	init_node(buf->d, kind, buf->blknum);
	return buf;
}

Buf_s *br_new(Htree_s *t)
{
FN;
	++t->stat.new_branches;
	return node_new(t, BRANCH);
}

Buf_s *lf_new(Htree_s *t)
{
FN;
	++t->stat.new_leaves;
	return node_new(t, LEAF);
}

Buf_s *lf_split(Buf_s *buf)
{
FN;
	Node_s *node    = buf->d;
	Htree_s *t      = buf->user;
	Buf_s *bsibling = lf_new(t);
	Node_s *sibling = bsibling->d;
	int middle      = (node->num_recs + 1) / 2;
	int i;

	LF_AUDIT(node);
	for (i = 0; middle < node->num_recs; i++) {
		lf_rec_move(sibling, i, node, middle);
	}
	node->last = bsibling->blknum;
	node->is_split = TRUE;
	LF_AUDIT(node);
	LF_AUDIT(sibling);
	++t->stat.split_leaf;
	return bsibling;
}

int lf_insert(Op_s *op)
{
FN;
	Buf_s *right;
	Node_s *leaf = op->buf->d;
	int size     = sizeof(op->r.key) + op->val.size + SZ_LEAF_OVERHEAD;
	int i;

	LF_AUDIT(leaf);
	while (size > leaf->free) {
		right = lf_split(op->buf);
		if (isLE(right->d, op->r.key, 0)) {
			buf_put_dirty(&right);
		} else {
			buf_put_dirty(&op->buf);
			op->buf = right;
			leaf = op->buf->d;
		}
	}
	if (size > usable(leaf)) {
		lf_compact(leaf);
	}
	i = leaf_le(leaf, op->r.key);
	store_rec(leaf, op->r, i);
	LF_AUDIT(leaf);
	buf_put_dirty(&op->buf);
	return 0;
}

int grow(Op_s *op)
{
FN;
	Node_s *node = op->buf->d;
	Node_s *parent;
	Twig_s twig;

	op->parent = br_new(op->tree);
	parent = op->parent->d;
	parent->last = node->last;

	twig.key = get_key(node, node->num_recs - 1);
	twig.blknum = node->blknum;
	store_twig(parent, twig, 0);

	if (node->kind == LEAF) {
		node->last = 0;
	} else {
		node->last = get_blknum(node, node->num_recs - 1);
		br_twig_del(node, node->num_recs - 1);
	}
	node->is_split = FALSE;
	op->tree->root = parent->blknum;
	buf_put_dirty(&op->buf);
	op->buf = op->parent;
	op->parent = NULL;
	return 0;
}

int br_split(Op_s *op)
{
FN;
	Node_s *parent = op->parent->d;
	Node_s *sibling;
	int middle = (parent->num_recs + 1) / 2;
	int i;

	op->sibling = br_new(op->tree);
	sibling = op->sibling->d;
	BR_AUDIT(parent);
	for (i = 0; middle < parent->num_recs; i++) {
		br_twig_move(sibling, i, parent, middle);
	}
	sibling->last = parent->last;
	parent->num_recs = middle;
	parent->last     = sibling->blknum;
	parent->is_split = TRUE;
	BR_AUDIT(parent);
	BR_AUDIT(sibling);
	++op->tree->stat.split_branch;
	return 0;
}

int br_reinsert(Op_s *op)
{
FN;
	Node_s *parent = op->parent->d;
	Node_s *node  = op->buf->d;
	int size;
	Twig_s twig;

	twig.key = get_key(node, node->num_recs - 1);
	size = twig.key.size + SZ_U64 + SZ_LEAF_OVERHEAD;

	while (size > parent->free) {
		br_split(op);

		if (isLE(parent, twig.key, parent->num_recs - 1)) {
			buf_put_dirty(&op->sibling);
		} else {
			buf_put_dirty(&op->parent);
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

	if (node->num_recs == 0) {
		/* We have a degenerate case */
		if (op->irec == parent->num_recs) {
			parent->last = node->last;
		} else {
			update_blknum(parent, node->last, op->irec);
		}
		buf_free(&op->buf);
		return 0;
	}
	twig.key = get_key(node, node->num_recs - 1);
	size = sizeof(twig.key) + SZ_U64 + SZ_LEAF_OVERHEAD;

	while (size > parent->free) {
		br_split(op);

		if (isLE(parent, twig.key, parent->num_recs - 1)) {
			buf_put_dirty(&op->sibling);
		} else {
			buf_put_dirty(&op->parent);
			op->parent  = op->sibling;
			op->sibling = NULL;
			parent      = op->parent->d;
			op->irec    = leaf_le(parent, twig.key);
		}
	}
	if (size > usable(parent)) {
		br_compact(parent);
	}
	if (op->irec == parent->num_recs) {
		twig.blknum = parent->last;
		parent->last = node->last;
	} else {
		update_blknum(parent, node->last, op->irec);
		twig.blknum = node->blknum;
	}
	store_twig(parent, twig, op->irec);
	if (node->kind == BRANCH) {
		node->last = get_blknum(node, node->num_recs - 1);
		br_twig_del(node, node->num_recs - 1);
	} else {
		node->last = 0;
	}
	node->is_split = FALSE;
	buf_put_dirty(&op->buf);
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
		if (op->irec == parent->num_recs) {
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
		buf_put(&op->parent);
		if (node->kind == LEAF) {
			lf_insert(op);
			return 0;
		}
		op->parent = op->buf;
		op->buf = NULL;
	}
}

int t_insert(Htree_s *t, Key_t key, Lump_s val)
{
FN;
	Op_s op = { t, NULL, NULL, NULL, 0, {key, val}, { 0 } };
	Node_s *node;
	int rc;

	if (t->root) {
		op.buf = t_get(t, t->root);
	} else {
		op.buf = lf_new(t);
		t->root = op.buf->blknum;
	}
	node = op.buf->d;
	if (node->is_split) {
		if (grow(&op)) return op.err.err;
buf_dirty(op.buf);
		node = op.buf->d;
	}
	if (node->kind == LEAF) {
		rc = lf_insert(&op);
	} else {
		rc = br_insert(&op);
	}
	cache_balanced(t->cache);
	++t->stat.insert;
	return rc;
}

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
		if (i == node->num_recs) {
			if (node->is_split) {
				right = t_get(bleaf->user, node->last);
				buf_put(&bleaf);
				bleaf = right;
				node = bleaf->d;
			} else {
				return HT_ERR_NOT_FOUND;
			}
		} else {
			v = get_val(node, i);
			*val = duplump(v);
			buf_put(&bleaf);
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
		if (x == parent->num_recs) {
			blknum = parent->last;
		} else {
			blknum = get_blknum(parent, x);
		}
		buf = t_get(bparent->user, blknum);
		node = buf->d;
		if (node->kind == LEAF) {
			buf_put_dirty(&bparent);
			lf_find(buf, key, val);
			return 0;
		}
		buf_put_dirty(&bparent);
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
	if (node->kind == LEAF) {
		rc = lf_find(buf, key, val);
	} else {
		rc = br_find(buf, key, val);
	}
	cache_balanced(t->cache);
	if (!rc) ++t->stat.find;
	return rc;
}

int lf_delete(Op_s *op)
{
	Buf_s *right;
	Node_s *node = op->buf->d;
	int i;

	for (;;) {
FN;
		i = leaf_eq(node, op->r.key);
		if (i == -1) {
			return ERR(HT_ERR_NOT_FOUND);
		}
		if (i == node->num_recs) {
			if (node->is_split) {
				right = t_get(op->tree, node->last);
				buf_put_dirty(&op->buf);
				op->buf = right;
				node = op->buf->d;
			} else {
				return ERR(HT_ERR_NOT_FOUND);
			}
		} else {
			delete_rec(node, i);
			buf_put_dirty(&op->buf);
			return 0;
		}
	}
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
	if (node->kind == LEAF) {
		if (inuse(node) > sibling->free) {
			return FALSE;
		}
		lf_compact(sibling);
		for (i = 0; node->num_recs; i++) {
			lf_rec_move(sibling, i, node, 0);
		}
	} else {
		Twig_s twig;
		int used = inuse(node);

		twig.key = get_key(parent, op->irec);
		used += sizeof(twig.key) + SZ_U64 + SZ_BRANCH_OVERHEAD;
		if (used > sibling->free) {
			return FALSE;
		}
		br_compact(sibling);
		for (i = 0; node->num_recs; i++) {
			br_twig_move(sibling, i, node, 0);
		}
		twig.blknum = node->last;
		store_twig(sibling, twig, i);
	}
	br_twig_del(parent, op->irec);
	buf_free(&op->buf);
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
		lf_rec_move(node, node->num_recs, sibling, 0);
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
	twig = get_twig(parent, op->irec);
	twigblknum = twig.blknum;
	twig.blknum = node->last;
	store_twig(node, twig, node->num_recs);
	br_twig_del(parent, op->irec);
	for (;;) {
		twig = get_twig(sibling, 0);
		if (node->free <= sibling->free) break;
		store_twig(node, twig, node->num_recs);
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

	if (op->irec == parent->num_recs) {
		/* No right sibling */
		return 0;
	}
	y = op->irec + 1;
	if (y == parent->num_recs) {
		blknum = parent->last;
	} else {
		blknum = get_blknum(parent, y);
	}
	op->sibling = t_get(op->tree, blknum);
	sibling = op->sibling->d;

	if (join(op)) return 0;

	if (sibling->num_recs == 0 && sibling->is_split) {
		// TODO(taysom) may want to do some thing with sibling?
		buf_put(&op->sibling);
		return 0;
	}
	assert(sibling->num_recs > 0);
	if (node->kind == LEAF) {
		lf_rebalance(op);
	} else {
		br_rebalance(op);
	}
	buf_dirty(op->parent);
	buf_dirty(op->buf);
	buf_put_dirty(&op->sibling);
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
		if (op->irec == parent->num_recs) {
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
		if (node->free > REBALANCE) {
			if (rebalance(op)) return FAILURE;
			node = op->buf->d;
		}
		buf_put(&op->parent);
		if (node->kind == LEAF) {
			lf_delete(op);
			return 0;
		}
		op->parent = op->buf;
		op->buf = NULL;
	}
}

int t_delete(Htree_s *t, Key_t key)
{
FN;
	Op_s op = { t, NULL, NULL, NULL, 0, {key, { 0 }}, { 0 } };
	Node_s *node;
	int rc;

	if (!t->root) {
		return HT_ERR_NOT_FOUND;
	}
	op.buf = t_get(t, t->root);
	node = op.buf->d;
	if (node->kind == LEAF) {
		rc = lf_delete(&op);
	} else {
		rc = br_delete(&op);
	}
	cache_balanced(t->cache);
	if (!rc) ++t->stat.delete;
	return rc;
}

Htree_s *t_new(char *file, int num_bufs)
{
FN;
	Htree_s *t;

	t = ezalloc(sizeof(*t));
	t->cache = cache_new(file, num_bufs, SZ_BUF);
	return t;
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

bool pr_key (Node_s *node, unint i)
{
	unint size;
	unint x;
	u8 *start;

	if (i >= node->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, node->num_recs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= SZ_BUF) {
		printf("key size at %ld offset %ld is too big %ld\n",
			i, x, size);
		return FALSE;
	}
	printf(" %4ld, %4ld:", x, size);
	pr_lump(lumpmk(size, start));
	putchar('\n');
	return TRUE;
}

bool pr_val (Node_s *node, unint i)
{
	unint key_size;
	unint size;
	unint x;
	u8 *start;

	if (i >= node->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, node->num_recs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	if (x + key_size >= SZ_BUF) {
		printf("key size at %ld offset %ld is too big %ld\n",
			i, x, key_size);
		return FALSE;
	}
	start += key_size;
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= SZ_BUF) {
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

	if (i >= node->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, node->num_recs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= SZ_BUF) {
		printf("key size at %ld offset %ld is too big %ld\n",
			i, x, size);
		return FALSE;
	}
	printf(" %4ld, %4ld:", x, size);
	pr_lump(lumpmk(size, start));
	start += size;
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= SZ_BUF) {
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

	pr_node(node, 0);
	for (i = 0; i < node->num_recs; i++) {
		pr_record(node, i);
	}
}

bool pr_twig (Node_s *node, unint i)
{
	unint size;
	unint x;
	u8 *start;
	Blknum_t blknum;

	if (i >= node->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, node->num_recs);
		return FALSE;
	}
	x = node->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)node)[x];
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= SZ_BUF) {
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

	pr_node(node, 0);
	for (i = 0; i < node->num_recs; i++) {
		pr_twig(node, i);
	}
	printf(" last: %lld\n", node->last);
}

void pr_node(Node_s *node)
{
	if (node->kind == LEAF) {
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

	for (i = 0; i < node->num_recs; i++) {
		rec = get_rec(node, i);
		printf("%4ld. ", Recnum++);
		lump_dump(rec.key);
		printf("\t");
		lump_dump(rec.val);
		printf("\n");
	}
	if (node->is_split) {
		pr_all_nodes(buf->user, node->last);
	}
}

static void pr_all_branches(Buf_s *buf)
{
	Node_s *node = buf->d;
	Blknum_t blknum;
	unint i;

	for (i = 0; i < node->num_recs; i++) {
		blknum = get_blknum(node, i);
		pr_all_nodes(buf->user, blknum);
	}
	pr_all_nodes(buf->user, node->last);
}

static void pr_all_nodes(Htree_s *t, Blknum_t blknum)
{
	Buf_s *buf;
	Node_s *node;

	if (!blknum) return;
	buf = t_get(t, blknum);
	node = buf->d;
	switch (node->kind) {
	case LEAF:
		pr_all_leaves(buf);
		break;
	case BRANCH:
		pr_all_branches(buf);
		break;
	default:
		printf("unknown kind %d\n", node->kind);
		break;
	}
	buf_put(&buf);
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

	for (i = 0; i < node->num_recs; i++) {
		rec = get_rec(node, i);
		rc = apply.func(rec, apply.tree, apply.user);
		if (rc) return rc;
	}
	if (node->is_split) {
		return node_map(buf->user, node->last, apply);
	}
	return 0;
}

static int br_map(Buf_s *buf, Apply_s apply)
{
	Node_s *node = buf->d;
	Blknum_t blknum;
	unint i;
	int rc;

	for (i = 0; i < node->num_recs; i++) {
		blknum = get_blknum(node, i);
		rc = node_map(buf->user, blknum, apply);
		if (rc) return rc;
	}
	return node_map(buf->user, node->last, apply);
}

static int node_map(Htree_s *t, Blknum_t blknum, Apply_s apply)
{
	Buf_s *buf;
	Node_s *node;
	int rc = 0;

	if (!blknum) return rc;
	buf = t_get(t, blknum);
	node = buf->d;
	switch (node->kind) {
	case LEAF:
		rc = lf_map(buf, apply);
		break;
	case BRANCH:
		rc = br_map(buf, apply);
		break;
	default:
		warn("unknown kind %d", node->kind);
		rc = HT_ERR_BAD_NODE;
		break;
	}
	buf_put(&buf);
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
	Lump_s *old = user;

	if (cmplump(rec.key, *old) < 0) {
		t_dump(t);
		pr_lump(rec.key);
		printf(" <= ");
		pr_lump(*old);
		printf("  ");
		fatal("keys out of order");
		return FAILURE;
	}
	freelump(*old);
	*old = duplump(rec.key);
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
	for (i = 0; i < node->num_recs; i++) {
		rec = get_rec(node, i);
		rc = apply.func(rec, apply.user);
		if (rc) return rc;
	}
#endif
	audit->records += node->num_recs;
	if (node->is_split) {
		return node_audit(buf->user, node->last, audit, depth-1);
	}
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
	for (i = 0; i < node->num_recs; i++) {
		blknum = get_blknum(node, i);
		rc = node_audit(buf->user, blknum, audit, depth);
		if (rc) return rc;
	}
	return node_audit(buf->user, node->last, audit,
			node->is_split ? depth-1 : depth);
}

static int node_audit(Htree_s *t, Blknum_t blknum, Audit_s *audit, int depth)
{
	Buf_s *buf;
	Node_s *node;
	int rc = 0;

	if (!blknum) return rc;
	buf = t_get(t, blknum);
	node = buf->d;
	if (node->is_split) ++audit->splits;
	switch (node->kind) {
	case LEAF:
		rc = leaf_audit(buf, audit, depth+1);
		break;
	case BRANCH:
		rc = branch_audit(buf, audit, depth+1);
		break;
	default:
		warn("unknown kind %d", node->kind);
		rc = HT_ERR_BAD_NODE;
		break;
	}
	buf_put(&buf);
	return rc;
}


int t_audit (Htree_s *t, Audit_s *audit)
{
	Lump_s old = Nil;
	int rc;

	zero(*audit); 
	rc = t_map(t, map_rec_audit, NULL, &old);
	if (rc) {
		printf("AUDIT FAILED %d\n", rc);
		return rc;
	}
	rc = node_audit(t, t->root, audit, 0);
	return rc;
}
