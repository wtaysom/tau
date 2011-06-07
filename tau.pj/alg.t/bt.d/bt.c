/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

/*
 * Terminology:
 *	lump - a pointer with a size. Nil is <0, NULL>. Notice that
 *		lumps are passed by value which means the size and
 *		the pointer are copied onto the stack.
 *	key - a lump. Index for a record.
 *	val - a lump. Value of a record.
 *	rec - <key, val>
 *	twig - <key, block>
 *	leaf - where the records of a the B-tree are stored.
 *	branch - Store indexes to to leaves on other branches
 */
// XXX: Todo
// 1. Combine lf_audit and leaf_audit
// 2. Timer loop to measure rates
// 3. Measure hit rate in cache
// 4. Plot results
// 5. Log
// 6. Transactions
// 7. Dependency graph
// 8. Global statistics - buffers
// 9. Join
// *10. Rebalance
// 11. Prefix optimization
// 12. Optimistic concurrency control - this is for the read path, if
//	the version changes while reading, restart. But this may be
//	dangerous if a record is updated.
// 13. Multiple test threads
// 14. Need iterator that returns key and val, I think that is what map does.
// 15. Only dirty buffers when needed.
// 16. Only flush dirty buffers when needed.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <mylib.h>
#include <style.h>

#include "bt.h"
#include "bt_disk.h"
#include "buf.h"

#define LF_AUDIT(_h)	lf_audit(FN_ARG, _h)
#define BR_AUDIT(_h)	br_audit(FN_ARG, _h)

enum {	MAX_U16 = (1 << 16) - 1,
	BITS_U8 = 8,
	MASK_U8 = (1 << BITS_U8) - 1,
	SZ_BUF  = 128,
	SZ_U16  = sizeof(u16),
	SZ_U64  = sizeof(u64),
	SZ_HEAD = sizeof(Head_s),
	SZ_LEAF_OVERHEAD   = 3 * SZ_U16,
	SZ_BRANCH_OVERHEAD = 2 * SZ_U16,
	REBALANCE = 2 * (SZ_BUF - SZ_HEAD) / 3 };

typedef struct Err_s {
	char	*where;
	int	rc;
} Err_s;

#define SET_ERR(_e, _rc) (((_e).where = WHERE), ((_e).rc = (_rc)))

struct Btree_s {
	Cache_s	*cache;
	u64	root;
	Stat_s	stat;
};

typedef struct Twig_s {
	Lump_s	key;
	u64	block;
} Twig_s;

typedef struct Audit_s {
	u64	leaves;
	u64	branches;
	u64	records;
	u64	splits;
} Audit_s;

typedef struct Apply_s {
	Apply_f	func;
	void	*user;
} Apply_s;

typedef struct Op_s {
	Btree_s	*tree;
	Buf_s	*parent;
	Buf_s	*child;
	int	irec;		/* Record index */
	Lump_s	key;
	Lump_s	val;
	Err_s	err;
} Op_s;

static inline Apply_s mk_apply(Apply_f func, void *user)
{
	Apply_s	a;

	a.func = func;
	a.user = user;
	return a;
}

bool pr_op(const char *fn, unsigned line, const char *label, Op_s *op)
{
	return print(fn, line,
		"%s: tree=%p parent=%p child=%p irec=%d key=%.*s val=%.*s err=%d",
		label, op->tree, op->parent, op->child, op->irec, op->key, op->val, op->err.rc);
}

#define PRop(_op)	pr_op(FN_ARG, # _op, _op)

void lump_dump(Lump_s a);
void lf_dump(Buf_s *node, int indent);
void br_dump(Buf_s *node, int indent);
void node_dump(Btree_s *t, u64 block, int indent);
void pr_leaf(Head_s *head);
void pr_branch(Head_s *head);
void pr_node(Head_s *head);

bool Dump_buf = FALSE;

Stat_s t_get_stats (Btree_s *t) { return t->stat; }

int usable(Head_s *head)
{
FN;
	return head->end - SZ_HEAD - SZ_U16 * head->num_recs;
}

int inuse(Head_s *head)
{
	return SZ_BUF - SZ_HEAD - head->free;
}

void pr_indent(int indent)
{
	int	i;

	for (i = 0; i < indent; i++) {
		printf("  ");
	}
}

void pr_head(Head_s *head, int indent)
{
	pr_indent(indent);
	printf("%s "
		"%s"
		"%lld num_recs %d "
		"free %d "
		"end %d "
		"last %lld\n",
		head->kind == LEAF ? "LEAF" : "BRANCH",
		head->is_split ? "*split* " : "",
		head->block,
		head->num_recs,
		head->free,
		head->end,
		head->last);
}

void pr_buf(Buf_s *buf, int indent)
{
	enum { BYTES_PER_ROW = 16 };
	unint	i;
	unint	j;
	u8	*d = buf->d;
	char	*c = buf->d;

	pr_indent(indent);
	printf("block=%lld\n", buf->block);
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
	int	i;

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

void pr_rec (Rec_s rec)
{
	pr_lump(rec.key);
	printf(" = ");
	pr_lump(rec.val);
}

void init_head(Head_s *head, u8 kind, u64 block)
{
FN;
	head->kind     = kind;
	head->num_recs = 0;
	head->is_split = FALSE;
	head->free     = SZ_BUF - SZ_HEAD;
	head->end      = SZ_BUF;
	head->block    = block;
	head->last     = 0;
}

void init_node(Buf_s *node, Btree_s *t, u8 kind)
{
FN;
	node->user = t;
	init_head(node->d, kind, node->block);
}

Lump_s get_key (Head_s *head, unint i)
{
	Lump_s	key;
	unint	x;
	u8	*start;

if (i >= head->num_recs) {
	PRd(i);
	PRd(head->num_recs);
	pr_node(head);
}
	assert(i < head->num_recs);
	x = head->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	key.size = *start++;
	key.size |= (*start++) << 8;
	key.d = start;
	return key;
}

Lump_s get_val (Head_s *head, unint i)
{
	Lump_s	val;
	unint	x;
	u8	*start;
	unint	key_size;

	assert(head->kind == LEAF);
	assert(i < head->num_recs);
	x = head->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	start += key_size;
	val.size = *start++;
	val.size |= (*start++) << 8;
	val.d = start;
	return val;
}

Rec_s get_rec (Head_s *head, unint i)
{
	Rec_s	rec;
	unint	x;
	u8	*start;

	assert(head->kind == LEAF);
	assert(i < head->num_recs);
	x = head->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	rec.key.size = *start++;
	rec.key.size |= (*start++) << 8;
	rec.key.d = start;
	start += rec.key.size;
	rec.val.size = *start++;
	rec.val.size |= (*start++) << 8;
	rec.val.d = start;
	return rec;
}

u64 get_block(Head_s *head, unint a)
{
FN;
	u64	block;
	unint	x;
	u8	*start;
	unint	key_size;

	assert(head->kind == BRANCH);
	assert(a < head->num_recs);
	x = head->rec[a];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	start += key_size;
	UNPACK(start, block);
	return block;
}

Twig_s get_twig(Head_s *head, unint a)
{
FN;
	Twig_s	twig;
	u8	*start;
	unint	x;

	assert(head->kind == BRANCH);
	assert(a < head->num_recs);
	x = head->rec[a];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	twig.key.size = *start++;
	twig.key.size |= (*start++) << 8;
	twig.key.d = start;
	start += twig.key.size;
	UNPACK(start, twig.block);
	return twig;
}

void lump_dump(Lump_s a)
{
FN;
#if 0
	int	i;

	for (i = 0; i < a.size; i++) {
		putchar(a.d[i]);
	}
#else
	printf("%.*s", a.size, a.d);
#endif
}

void rec_dump (Rec_s rec)
{
	lump_dump(rec.key);
	printf(" = ");
	lump_dump(rec.val);
	printf("\n");
}

void lf_dump(Buf_s *node, int indent)
{
	Head_s	*head = node->d;
	Rec_s	rec;
	unint	i;

	if (Dump_buf) {
		pr_buf(node, indent);
	}
	pr_head(head, indent);
	for (i = 0; i < head->num_recs; i++) {
		rec = get_rec(head, i);
		pr_indent(indent);
		printf("%ld. ", i);
		rec_dump(rec);
	}
	if (head->is_split) {
		pr_indent(indent);
		printf("Leaf Split:\n");
		node_dump(node->user, head->last, indent);
	}
}

void br_dump(Buf_s *node, int indent)
{
	Head_s	*head = node->d;
	Twig_s	twig;
	unint	i;

	pr_head(head, indent);
	for (i = 0; i < head->num_recs; i++) {
		twig = get_twig(head, i);
		pr_indent(indent);
		printf("%ld. ", i);
		lump_dump(twig.key);
		printf(" = %lld\n", twig.block);
		node_dump(node->user, twig.block, indent + 1);
	}
	if (head->is_split) {
		pr_indent(indent);
		printf("Branch Split:\n");
		node_dump(node->user, head->last, indent);
	} else {
		pr_indent(indent);
		printf("%ld. <last> = %lld\n", i, head->last);
		node_dump(node->user, head->last, indent + 1);
	}
}

void node_dump(Btree_s *t, u64 block, int indent)
{
	Buf_s	*node;
	Head_s	*head;

	if (!block) return;
	node = buf_get(t->cache, block);
	head = node->d;
	switch (head->kind) {
	case LEAF:
		lf_dump(node, indent);
		break;
	case BRANCH:
		br_dump(node, indent + 1);
		break;
	default:
		printf("unknown kind %d\n", head->kind);
		break;
	}
	buf_put(&node);
}

#if 0
bool isGE(Head_s *head, Lump_s key, unint i)
{
FN;
	Lump_s	target;

	target = get_key(head, i);
	return cmplump(key, target) >= 0;
}
#endif

bool isLE(Head_s *head, Lump_s key, unint i)
{
FN;
	Lump_s	target;

	target = get_key(head, i);
	return cmplump(key, target) <= 0;
}

int isEQ(Head_s *head, Lump_s key, unint i)
{
FN;
	Lump_s	target;

	target = get_key(head, i);
	return cmplump(key, target);
}

void store_lump(Head_s *head, Lump_s lump)
{
FN;
	int	total = lump.size + SZ_U16;
	u8	*start;

	assert(total <= head->free);
	assert(total <= head->end);
	assert(lump.size < MAX_U16);

	head->end -= total;
	head->free -= total;
	start = &((u8 *)head)[head->end];
	*start++ = lump.size & MASK_U8;
	*start++ = (lump.size >> BITS_U8) & MASK_U8;
	memmove(start, lump.d, lump.size);
}

void store_block(Head_s *head, u64 block)
{
FN;
	int	total = SZ_U64;
	u8	*start;

	assert(total <= head->free);
	assert(total <= head->end);

	head->end -= total;
	head->free -= total;
	start = &((u8 *)head)[head->end];
	PACK(start, block);
}

void update_block(Head_s *head, u64 block, unint a)
{
FN;
	unint	x;
	u8	*start;
	unint	key_size;

	assert(head->kind == BRANCH);
	assert(a < head->num_recs);
	x = head->rec[a];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	start += key_size;
	PACK(start, block);
}

/* XXX: Don't like this name */
void store_end(Head_s *head, unint i)
{
FN;
	assert(i <= head->num_recs);
	if (i < head->num_recs) {
		memmove(&head->rec[i+1], &head->rec[i],
			(head->num_recs - i) * SZ_U16);
	}
	++head->num_recs;
	head->rec[i] = head->end;
	head->free -= SZ_U16;
}

void _store_rec(Head_s *head, Lump_s key, Lump_s val, unint i)
{
FN;
	store_lump(head, val);
	store_lump(head, key);
	store_end(head, i);
}

void rec_copy(Head_s *dst, int a, Head_s *src, int b)
{
	Rec_s	rec;

	rec = get_rec(src, b);

	store_lump(dst, rec.val);
	store_lump(dst, rec.key);
	store_end(dst, a);
}

void twig_copy(Head_s *dst, int a, Head_s *src, int b)
{
	Twig_s	twig;

	twig = get_twig(src, b);

	store_block(dst, twig.block);
	store_lump(dst, twig.key);
	store_end(dst, a);
}

void lf_compact(Head_s *leaf)
{
FN;
	u8	b[SZ_BUF];
	Head_s	*h = (Head_s *)b;
	int	i;

	if (usable(leaf) == leaf->free) {
		return;
	}
	init_head(h, LEAF, leaf->block);
	h->is_split  = leaf->is_split;
	h->last      = leaf->last;
	for (i = 0; i < leaf->num_recs; i++) {
		rec_copy(h, i, leaf, i);
	}
	memmove(leaf, h, SZ_BUF);
}

void br_compact(Head_s *branch)
{
FN;
	u8	b[SZ_BUF];
	Head_s	*h = (Head_s *)b;
	int	i;

	if (usable(branch) == branch->free) {
		return;
	}
	init_head(h, BRANCH, branch->block);
	h->is_split  = branch->is_split;
	h->last      = branch->last;
	for (i = 0; i < branch->num_recs; i++) {
		twig_copy(h, i, branch, i);
	}
	memmove(branch, h, SZ_BUF);
}

void store_rec(Head_s *head, Lump_s key, Lump_s val, unint i)
{
FN;
	lf_compact(head);
	store_lump(head, val);
	store_lump(head, key);
	store_end(head, i);
}

void store_twig(Head_s *head, Twig_s twig, unint i)
{
FN;
	br_compact(head);
	store_block(head, twig.block);
	store_lump(head, twig.key);
	store_end(head, i);
}

void delete_rec(Head_s *head, unint i)
{
FN;
	Rec_s	rec;

	assert(i < head->num_recs);
	assert(head->kind == LEAF);
	rec = get_rec(head, i);
	head->free += rec.key.size + rec.val.size + SZ_LEAF_OVERHEAD;
	--head->num_recs;
	if (i < head->num_recs) {
		memmove(&head->rec[i], &head->rec[i+1],
			(head->num_recs - i) * SZ_U16);
	}
}

void lf_audit(const char *fn, unsigned line, Head_s *head)
{
FN;
	Rec_s	rec;
	int	free = SZ_BUF - SZ_HEAD;
	int	i;

	assert(head->kind == LEAF);
	for (i = 0; i < head->num_recs; i++) {
		rec = get_rec(head, i);
		free -= rec.key.size + rec.val.size + 3 * SZ_U16;
	}
	if (free != head->free) {
		printf("%s<%d> free:%d != %d:head->free\n",
			fn, line, free, head->free);
		exit(2);
	}
}

void br_audit(const char *fn, unsigned line, Head_s *head)
{
FN;
	int	i;
	int	free = SZ_BUF - SZ_HEAD;

	assert(head->kind == BRANCH);
	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;

		key = get_key(head, i);
		free -= key.size + SZ_U64 + 2 * SZ_U16;
	}
	if (free != head->free) {
		printf("%s<%d> free:%d != %d:head->free\n",
			fn, line, free, head->free);
		exit(2);
	}
}

int find_le(Head_s *head, Lump_s key)
{
FN;
	int	x;
	int	left;
	int	right;

	left = 0;
	right = head->num_recs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (isLE(head, key, x)) {
			right = x - 1;
		} else {
			left = x + 1;
		}
	}
	return left;
}

#if 0
int find_ge(Head_s *head, Lump_s key)
{
FN;
	int	x;
	int	left;
	int	right;

	left = 0;
	right = head->num_recs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		if (isGE(head, key, x)) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	return right;
}
#endif

int find_eq(Head_s *head, Lump_s key)
{
FN;
	int	x;
	int	left;
	int	right;
	int	eq;

	left = 0;
	right = head->num_recs - 1;
	while (left <= right) {
		x = (left + right) / 2;
		eq = isEQ(head, key, x);
		if (eq == 0) {
			return x;
		} else if (eq > 0) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	if (left == head->num_recs) return left;
	return -1;
}

void lf_del_rec(Head_s *head, unint i)
{
FN;
	Rec_s	rec;

	LF_AUDIT(head);
	assert(i < head->num_recs);
	rec = get_rec(head, i);
	head->free += rec.key.size + rec.val.size + 3 * SZ_U16;

	--head->num_recs;
	if (i < head->num_recs) {
		memmove(&head->rec[i], &head->rec[i+1],
			(head->num_recs - i) * SZ_U16);
	}
	LF_AUDIT(head);
}

void lf_rec_copy(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Rec_s	rec;

	rec = get_rec(src, j);

	store_rec(dst, rec.key, rec.val, i);
}

void lf_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Rec_s	rec;

	rec = get_rec(src, j);

	store_rec(dst, rec.key, rec.val, i);

	lf_del_rec(src, j);
}

#if 0
void lf_compact(Buf_s *bleaf)
{
FN;
	Head_s	*head = bleaf->d;
	Buf_s	*b;
	Head_s	*h;
	int	i;

	if (usable(head) == head->free) {
		return;
	}
	b = buf_scratch(bleaf->cache);
	h = b->d;
	init_node(b, bleaf->user, LEAF);
	h->is_split  = head->is_split;
	h->block     = head->block;
	h->last      = head->last;
	for (i = 0; i < head->num_recs; i++) {
		lf_rec_copy(h, i, head, i);
	}
	memmove(head, h, SZ_BUF);
	buf_toss(b);
}
#endif

void br_del_rec(Head_s *head, unint i)
{
FN;
	Lump_s	key;

	BR_AUDIT(head);
	assert(i < head->num_recs);
	key = get_key(head, i);
	head->free += key.size + SZ_U64 + 2 * SZ_U16;

	--head->num_recs;
	if (i < head->num_recs) {
		memmove(&head->rec[i], &head->rec[i+1],
			(head->num_recs - i) * SZ_U16);
	}
	BR_AUDIT(head);
}

void br_rec_copy(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Twig_s	twig;

	twig = get_twig(src, j);

	store_twig(dst, twig, i);
}

void br_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Twig_s	twig;

	twig = get_twig(src, j);
	store_twig(dst, twig, i);

	br_del_rec(src, j);
}

Buf_s *node_new(Btree_s *t, u8 kind)
{
FN;
	Buf_s	*node;

	node = buf_new(t->cache);
	init_node(node, t, kind);
	return node;
}

Buf_s *br_new(Btree_s *t)
{
FN;
	++t->stat.new_branches;
	return node_new(t, BRANCH);
}

Buf_s *lf_new(Btree_s *t)
{
FN;
	++t->stat.new_leaves;
	return node_new(t, LEAF);
}

Buf_s *lf_split(Buf_s *bchild)
{
FN;
	Head_s	*child    = bchild->d;
	Btree_s	*t        = bchild->user;
	Buf_s	*bsibling = lf_new(t);
	Head_s	*sibling  = bsibling->d;
	int	middle    = (child->num_recs + 1) / 2;
	int	i;

	LF_AUDIT(child);
	for (i = 0; middle < child->num_recs; i++) {
		lf_rec_move(sibling, i, child, middle);
	}
	child->last = bsibling->block;
	child->is_split = TRUE;
	LF_AUDIT(child);
	LF_AUDIT(sibling);
	++t->stat.split_leaf;
	return bsibling;
}

int lf_insert(Op_s *op)
{
FN;
	Buf_s	*right;
	Head_s	*leaf = op->child->d;
	int	size  = op->key.size + op->val.size + SZ_LEAF_OVERHEAD;
	int	i;

	LF_AUDIT(leaf);
	while (size > leaf->free) {
		right = lf_split(op->child);
		if (isLE(right->d, op->key, 0)) {
			buf_put(&right);
		} else {
			buf_put(&op->child);
			op->child = right;
			leaf = op->child->d;
		}
	}
	if (size > usable(leaf)) {
		lf_compact(leaf);
	}
	i = find_le(leaf, op->key);
	store_rec(leaf, op->key, op->val, i);
	LF_AUDIT(leaf);
	buf_put(&op->child);
	return 0;
}

int grow(Op_s *op)
{
FN;
	Head_s	*child = op->child->d;
	Head_s	*parent;
	Twig_s	twig;

	op->parent = br_new(op->tree);
	parent = op->parent->d;
	parent->last = child->last;

	twig.key = get_key(child, child->num_recs - 1);
	twig.block = child->block;
	store_twig(parent, twig, 0);

	if (child->kind == LEAF) {
		child->last = 0;
	} else {
		child->last = get_block(child, child->num_recs - 1);
		br_del_rec(child, child->num_recs - 1);
	}
	child->is_split = FALSE;
	op->tree->root = parent->block;
	buf_put(&op->child);
	op->child = op->parent;
	op->parent = NULL;
	return 0;
}

Buf_s *br_split(Buf_s *bchild)
{
FN;
	Head_s	*child    = bchild->d;
	Btree_s	*t        = bchild->user;
	Buf_s	*bsibling = br_new(t);
	Head_s	*sibling  = bsibling->d;
	int	middle    = (child->num_recs + 1) / 2;
	int	i;

	BR_AUDIT(child);
	for (i = 0; middle < child->num_recs; i++) {
		br_rec_move(sibling, i, child, middle);
	}
	sibling->last = child->last;
	child->num_recs = middle;
	child->last     = sibling->block;
	child->is_split = TRUE;
	BR_AUDIT(child);
	BR_AUDIT(sibling);
	++t->stat.split_branch;
	return bsibling;
}

Buf_s *br_reinsert(Buf_s *bparent, Buf_s *bchild, int x)
{
FN;
	Head_s	*parent = bparent->d;
	Head_s	*child  = bchild->d;
	int	size;
	Twig_s	twig;

	twig.key = get_key(child, child->num_recs - 1);
	size = twig.key.size + SZ_U64 + SZ_LEAF_OVERHEAD;

	while (size > parent->free) {
HERE;
		Buf_s	*right = br_split(bparent);

		if (isLE(parent, twig.key, parent->num_recs - 1)) {
			buf_put(&right);
		} else {
			buf_put(&bparent);
			bparent = right;
			parent  = bparent->d;
		}
		x = find_le(parent, twig.key);
	}
	if (size > usable(parent)) {
		br_compact(parent);
	}
	twig.block = child->block;
	store_twig(parent, twig, x);
	return bparent;
}

int br_store(Op_s *op)
{
FN;
	Head_s	*parent = op->parent->d;
	Head_s	*child  = op->child->d;
	int	size;
	Twig_s	twig;

	if (child->num_recs == 0) {
		/* We have a degenerate case */
		update_block(parent, child->last, op->irec);
		buf_free(&op->child);
		return 0;
	}
	twig.key = get_key(child, child->num_recs - 1);
	size = twig.key.size + SZ_U64 + SZ_LEAF_OVERHEAD;

	while (size > parent->free) {
		Buf_s	*right = br_split(op->parent);

		if (isLE(parent, twig.key, parent->num_recs - 1)) {
			buf_put(&right);
		} else {
			buf_put(&op->parent);
			op->parent = right;
			parent  = op->parent->d;
			op->irec = find_le(parent, twig.key);
		}
	}
	if (size > usable(parent)) {
		br_compact(parent);
	}
	if (op->irec == parent->num_recs) {
		twig.block = parent->last;
		parent->last = child->last;
	} else {
		update_block(parent, child->last, op->irec);
		twig.block = child->block;
	}
	store_twig(parent, twig, op->irec);
	if (child->kind == BRANCH) {
		child->last = get_block(child, child->num_recs - 1);
		br_del_rec(child, child->num_recs - 1);
	} else {
		child->last = 0;
	}
	child->is_split = FALSE;
	buf_put(&op->child);
	return 0;
}

int br_insert(Op_s *op)
{
FN;
	Head_s	*parent;
	Head_s	*child;
	u64	block;

	op->parent = op->child;
	op->child = NULL;
	for(;;) {
		parent = op->parent->d;
		op->irec = find_le(parent, op->key);
		if (op->irec == parent->num_recs) {
			block = parent->last;
		} else {
			block = get_block(parent, op->irec);
		}
		op->child = buf_get(op->tree->cache, block);
		child = op->child->d;
		if (child->is_split) {
			if (br_store(op)) return FAILURE;
			continue;
		}
		buf_put(&op->parent);
		if (child->kind == LEAF) {
			lf_insert(op);
			return 0;
		}
		op->parent = op->child;
		op->child = NULL;
	}
}

int t_insert(Btree_s *t, Lump_s key, Lump_s val)
{
FN;
	Op_s	op = { t, NULL, NULL, 0, key, val, { 0 } };
	Head_s	*child;
	int	rc;

	if (t->root) {
		op.child = buf_get(t->cache, t->root);
	} else {
		op.child = lf_new(t);
		t->root = op.child->block;
	}
	child = op.child->d;
	if (child->is_split) {
		if (grow(&op)) return op.err.rc;
		child = op.child->d;
	}
	if (child->kind == LEAF) {
		rc = lf_insert(&op);
	} else {
		rc = br_insert(&op);
	}
	cache_balanced(t->cache);
	++t->stat.insert;
	return rc;
}

int lf_find(Buf_s *bleaf, Lump_s key, Lump_s *val)
{
FN;
	Buf_s	*right;
	Head_s	*head = bleaf->d;
	Lump_s	v;
	int	i;

	for (;;) {
		i = find_eq(head, key);
		if (i == -1) {
			return BT_ERR_NOT_FOUND;
		}
		if (i == head->num_recs) {
			if (head->is_split) {
				right = buf_get(bleaf->cache, head->last);
				buf_put(&bleaf);
				bleaf = right;
				head = bleaf->d;
			} else {
				return BT_ERR_NOT_FOUND;
			}
		} else {
			v = get_val(head, i);
			*val = duplump(v);
			buf_put(&bleaf);
			return 0;
		}
	}
}

int br_find(Buf_s *bparent, Lump_s key, Lump_s *val)
{
FN;
	Head_s	*parent = bparent->d;
	Buf_s	*bchild;
	Head_s	*child;
	u64	block;
	int	x;

	for(;;) {
		x = find_le(parent, key);
		if (x == parent->num_recs) {
			block = parent->last;
		} else {
			block = get_block(parent, x);
		}
		bchild = buf_get(bparent->cache, block);
		child = bchild->d;
		if (child->kind == LEAF) {
			buf_put(&bparent);
			lf_find(bchild, key, val);
			return 0;
		}
		buf_put(&bparent);
		bparent = bchild;
		parent = bparent->d;
	}
}

int t_find(Btree_s *t, Lump_s key, Lump_s *val)
{
FN;
	Buf_s	*node;
	Head_s	*head;
	int	rc;

	if (!t->root) {
		return BT_ERR_NOT_FOUND;
	}
	node = buf_get(t->cache, t->root);
	head = node->d;
	if (head->kind == LEAF) {
		rc = lf_find(node, key, val);
	} else {
		rc = br_find(node, key, val);
	}
	cache_balanced(t->cache);
	if (!rc) ++t->stat.find;
	return rc;
}

int lf_delete(Buf_s *bleaf, Lump_s key)
{
FN;
	Buf_s	*right;
	Head_s	*head = bleaf->d;
	int	i;

	for (;;) {
		i = find_eq(head, key);
		if (i == -1) {
			return BT_ERR_NOT_FOUND;
		}
		if (i == head->num_recs) {
			if (head->is_split) {
				right = buf_get(bleaf->cache, head->last);
				buf_put(&bleaf);
				bleaf = right;
				head = bleaf->d;
			} else {
				return BT_ERR_NOT_FOUND;
			}
		} else {
			delete_rec(head, i);
			buf_put(&bleaf);
			return 0;
		}
	}
}

Buf_s *join(Buf_s *bparent, int x, Buf_s *bchild, Buf_s *bsibling)
{
	Head_s	*parent  = bparent->d;
	Head_s	*child   = bchild->d;
	Head_s	*sibling = bsibling->d;
	int	i;

	if (child->kind == LEAF) {
		lf_compact(sibling);
		for (i = 0; child->num_recs; i++) {
PRd(child->num_recs);
			lf_rec_move(sibling, i, child, 0);
			if (child->free <= sibling->free) break;
		}
	} else {
PRd(child->num_recs);
//XXX: this is not quite write because it doesn't handle last
		br_compact(child);
		for (i = 0; child->num_recs; i++) {
			br_rec_move(sibling, i, child, 0);
		}
	}
	br_del_rec(parent, x);
	buf_free(&bchild);
	buf_put(&bsibling);
	return bparent;
}

Buf_s *rebalance(Buf_s *bparent, int x, Buf_s *bchild)
{
	Head_s	*parent = bparent->d;
	Head_s	*child  = bchild->d;
	Buf_s	*bsibling;
	Head_s	*sibling;
	u64	block;
	int	y;
	int	i;

	if (x == parent->num_recs) {
HERE;
		/* No right sibling */
		return bparent;
	}
	y = x + 1;
	if (y == parent->num_recs) {
		block = parent->last;
	} else {
		block = get_block(parent, y);
	}
	bsibling = buf_get(bparent->cache, block);
	sibling = bsibling->d;
	/* Can we join? */
	if (child->free > inuse(sibling)) {
HERE;
		return join(bparent, x, bchild, bsibling);
	}
	if (sibling->num_recs == 0) {
HERE;
		//XXX: should delete it
		buf_put(&bsibling);
		return bparent;
	}
	if (child->kind == LEAF) {
		lf_compact(child);
		for (i = 0; ;i++) {
			//XXX: May want to move more to the left
			// to compact things. For random, that might
			// be an interesting experiment.
			// May need to compact
			// Compacting first, certainly simplified things
			// Can I do a quick check for compaction? I think
			// so. That would simplify a lot of things.
			// Need to check if we even have room for the
			// record. They are variable length after all.
			lf_rec_move(child, child->num_recs, sibling, 0);
			if (child->free <= sibling->free) break;
		}
	} else {
		br_compact(child);
		for (i = 0; ;i++) {
			//XXX: see comments above.
			br_rec_move(child, child->num_recs, sibling, 0);
			if (child->free <= sibling->free) break;
		}
	}
	buf_put(&bsibling);
	buf_put(&bchild);
	br_del_rec(parent, x);
	bparent = br_reinsert(bparent, bchild, x);
HERE;
	return bparent;
}

int br_delete(Buf_s *bparent, Lump_s key)
{
FN;
	Head_s	*parent = bparent->d;
	Buf_s	*bchild;
	Head_s	*child;
	u64	block;
	int	x;

	for(;;) {
		x = find_le(parent, key);
		if (x == parent->num_recs) {
			block = parent->last;
		} else {
			block = get_block(parent, x);
		}
		bchild = buf_get(bparent->cache, block);
		child = bchild->d;
		if (child->is_split) {
#if 0
			bparent = br_store(bparent, bchild, x);
			parent = bparent->d;
			buf_put(&bchild);
			continue;
#endif
		} else if (child->free > REBALANCE) {
			bparent = rebalance(bparent, x, bchild);
			continue;	// Had to add this for join
		}
		if (child->kind == LEAF) {
			buf_put(&bparent);
			lf_delete(bchild, key);
			return 0;
		}
		buf_put(&bparent);
		bparent = bchild;
		parent = bparent->d;
	}
}

int t_delete(Btree_s *t, Lump_s key)
{
FN;
//	Op_s	op = { t, NULL, NULL, 0, key, { 0 }, { 0 } };
	Buf_s	*node;
	Head_s	*head;
	int	rc;

	if (!t->root) {
		return BT_ERR_NOT_FOUND;
	}
	node = buf_get(t->cache, t->root);
	head = node->d;
	if (head->kind == LEAF) {
		rc = lf_delete(node, key);
	} else {
		rc = br_delete(node, key);
	}
	cache_balanced(t->cache);
	if (!rc) ++t->stat.delete;
	return rc;
}

Btree_s *t_new(char *file, int num_bufs)
{
FN;
	Btree_s	*t;

	t = ezalloc(sizeof(*t));
	t->cache = cache_new(file, num_bufs, SZ_BUF);
	return t;
}

void t_dump(Btree_s *t)
{
	printf("**************************************************\n");
	node_dump(t, t->root, 0);
	cache_balanced(t->cache);
}

bool pr_key (Head_s *head, unint i)
{
	unint	size;
	unint	x;
	u8	*start;

	if (i >= head->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, head->num_recs);
		return FALSE;
	}
	x = head->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)head)[x];
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

bool pr_val (Head_s *head, unint i)
{
	unint	key_size;
	unint	size;
	unint	x;
	u8	*start;

	if (i >= head->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, head->num_recs);
		return FALSE;
	}
	x = head->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)head)[x];
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

bool pr_record (Head_s *head, unint i)
{
	unint	size;
	unint	x;
	u8	*start;

	if (i >= head->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, head->num_recs);
		return FALSE;
	}
	x = head->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)head)[x];
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

void pr_leaf(Head_s *head)
{
	unint	i;

	pr_head(head, 0);
	for (i = 0; i < head->num_recs; i++) {
		pr_record(head, i);
	}
}

bool pr_twig (Head_s *head, unint i)
{
	unint	size;
	unint	x;
	u8	*start;
	u64	block;

	if (i >= head->num_recs) {
		printf("key index >= num_recs %ld >= %d\n",
			i, head->num_recs);
		return FALSE;
	}
	x = head->rec[i];
	if (x >= SZ_BUF) {
		printf("key offset >= SZ_BUF %ld >= %ld\n",
			x, i);
		return FALSE;
	}
	start = &((u8 *)head)[x];
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

	UNPACK(start, block);
	printf(" : %lld", block);
	putchar('\n');
	return TRUE;
}

void pr_branch(Head_s *head)
{
	unint	i;

	pr_head(head, 0);
	for (i = 0; i < head->num_recs; i++) {
		pr_twig(head, i);
	}
	printf(" last: %lld\n", head->last);
}

void pr_node(Head_s *head)
{
	if (head->kind == LEAF) {
		pr_leaf(head);
	} else {
		pr_branch(head);
	}
}

static unint Recnum;

static void pr_all_nodes(Btree_s *t, u64 block);

static void pr_all_leaves(Buf_s *node)
{
	Head_s	*head = node->d;
	Rec_s	rec;
	unint	i;

	for (i = 0; i < head->num_recs; i++) {
		rec = get_rec(head, i);
		printf("%4ld. ", Recnum++);
		lump_dump(rec.key);
		printf("\t");
		lump_dump(rec.val);
		printf("\n");
	}
	if (head->is_split) {
		pr_all_nodes(node->user, head->last);
	}
}

static void pr_all_branches(Buf_s *node)
{
	Head_s	*head = node->d;
	u64	block;
	unint	i;

	for (i = 0; i < head->num_recs; i++) {
		block = get_block(head, i);
		pr_all_nodes(node->user, block);
	}
	pr_all_nodes(node->user, head->last);
}

static void pr_all_nodes(Btree_s *t, u64 block)
{
	Buf_s	*node;
	Head_s	*head;

	if (!block) return;
	node = buf_get(t->cache, block);
	head = node->d;
	switch (head->kind) {
	case LEAF:
		pr_all_leaves(node);
		break;
	case BRANCH:
		pr_all_branches(node);
		break;
	default:
		printf("unknown kind %d\n", head->kind);
		break;
	}
	buf_put(&node);
}

void pr_all_records(Btree_s *t)
{
	printf("=====All Records in Order=======\n");
	Recnum = 1;
	pr_all_nodes(t, t->root);
	cache_balanced(t->cache);
}

static int node_map(Btree_s *t, u64 block, Apply_s apply);

static int lf_map(Buf_s *node, Apply_s apply)
{
	Head_s	*head = node->d;
	unint	i;
	Rec_s	rec;
	int	rc;

	for (i = 0; i < head->num_recs; i++) {
		rec = get_rec(head, i);
		rc = apply.func(rec, apply.user);
		if (rc) return rc;
	}
	if (head->is_split) {
		return node_map(node->user, head->last, apply);
	}
	return 0;
}

static int br_map(Buf_s *node, Apply_s apply)
{
	Head_s	*head = node->d;
	u64	block;
	unint	i;
	int	rc;

	for (i = 0; i < head->num_recs; i++) {
		block = get_block(head, i);
		rc = node_map(node->user, block, apply);
		if (rc) return rc;
	}
	return node_map(node->user, head->last, apply);
}

static int node_map(Btree_s *t, u64 block, Apply_s apply)
{
	Buf_s	*node;
	Head_s	*head;
	int	rc = 0;

	if (!block) return rc;
	node = buf_get(t->cache, block);
	head = node->d;
	switch (head->kind) {
	case LEAF:
		rc = lf_map(node, apply);
		break;
	case BRANCH:
		rc = br_map(node, apply);
		break;
	default:
		warn("unknown kind %d", head->kind);
		rc = BT_ERR_BAD_NODE;
		break;
	}
	buf_put(&node);
	return rc;
}

int t_map(Btree_s *t, Apply_f func, void *user)
{
	Apply_s	apply = mk_apply(func, user);
	int	rc = node_map(t, t->root, apply);
	cache_balanced(t->cache);
	return rc;
}

static int rec_audit (Rec_s rec, void *user) {
	Lump_s	*old = user;

	if (cmplump(rec.key, *old) < 0) {
		pr_lump(rec.key);
		printf(" < ");
		pr_lump(*old);
		printf("  ");
		fatal("keys out of order");
		return FAILURE;
	}
	freelump(*old);
	*old = duplump(rec.key);
	return 0;
}

static int node_audit(Btree_s *t, u64 block, Audit_s *audit);

static int leaf_audit(Buf_s *node, Audit_s *audit)
{
	Head_s	*head = node->d;

	++audit->leaves;
#if 0
	unint	i;
	Rec_s	rec;
	int	rc;
	for (i = 0; i < head->num_recs; i++) {
		rec = get_rec(head, i);
		rc = apply.func(rec, apply.user);
		if (rc) return rc;
	}
#endif
	audit->records += head->num_recs;
	if (head->is_split) {
		return node_audit(node->user, head->last, audit);
	}
	return 0;
}

static int branch_audit(Buf_s *node, Audit_s *audit)
{
	Head_s	*head = node->d;
	u64	block;
	unint	i;
	int	rc;

	++audit->branches;
	for (i = 0; i < head->num_recs; i++) {
		block = get_block(head, i);
		rc = node_audit(node->user, block, audit);
		if (rc) return rc;
	}
	return node_audit(node->user, head->last, audit);
}

static int node_audit(Btree_s *t, u64 block, Audit_s *audit)
{
	Buf_s	*node;
	Head_s	*head;
	int	rc = 0;

	if (!block) return rc;
	node = buf_get(t->cache, block);
	head = node->d;
	if (head->is_split) ++audit->splits;
	switch (head->kind) {
	case LEAF:
		rc = leaf_audit(node, audit);
		break;
	case BRANCH:
		rc = branch_audit(node, audit);
		break;
	default:
		warn("unknown kind %d", head->kind);
		rc = BT_ERR_BAD_NODE;
		break;
	}
	buf_put(&node);
	return rc;
}


int t_audit (Btree_s *t) {
	Audit_s	audit = { 0 };
	Lump_s	old = Nil;
	int rc = t_map(t, rec_audit, &old);

	if (rc) {
		printf("AUDIT FAILED %d\n", rc);
		return rc;
	}
	rc = node_audit(t, t->root, &audit);
	return rc;
}
