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
// 10. Rebalance
// 11. Prefix optimization
// 12. Optimistic concurrency control - this is for the read path, if
//	the version changes while reading, restart. But this may be
//	dangerous if a record is updated.
// 13. Multiple test threads

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

struct Btree_s {
	Cache_s	*cache;
	u64	root;
	Stat_s	stat;	
};

typedef struct Rec_s {
	Lump_s	key;
	Lump_s	val;
} Rec_s;

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

static inline Apply_s mk_apply(Apply_f func, void *user)
{
	Apply_s	a;

	a.func = func;
	a.user = user;
	return a;
}

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
	unint	i;
	unint	j;
	u8	*d = buf->d;
	char	*c = buf->d;

	pr_indent(indent);
	printf("block=%lld\n", buf->block);
	for (i = 0; i < SZ_BUF / 16; i++) {
		pr_indent(indent);
		for (j = 0; j < 16; j++) {
			printf(" %.2x", *d++);
		}
		printf(" | ");
		for (j = 0; j < 16; j++) {
			putchar(isprint(*c) ? *c : '.');
			++c;
		}
		printf("\n");
	}
}

void init_node(Buf_s *node, Btree_s *t, u8 kind)
{
FN;
	Head_s	*head;

	node->user = t;
	head = node->d;
	head->kind     = kind;
	head->num_recs = 0;
	head->is_split = FALSE;
	head->free     = SZ_BUF - SZ_HEAD;
	head->end      = SZ_BUF;
	head->block    = node->block;
	head->last     = 0;
}

Lump_s get_key (Head_s *head, unint i)
{
	Lump_s	key;
	unint	x;
	u8	*start;

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

void lf_dump(Buf_s *node, int indent)
{
	Head_s	*head = node->d;
	unint	i;

	if (Dump_buf) {
		pr_buf(node, indent);
	}
	pr_head(head, indent);
	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;
		Lump_s val;

		key = get_key(head, i);
		val = get_val(head, i);
		pr_indent(indent);
		printf("%ld. ", i);
		lump_dump(key);
		printf(" = ");
		lump_dump(val);
		printf("\n");
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
	unint	i;

	pr_head(head, indent);
	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;
		u64 block;

		key   = get_key(head, i);
		block = get_block(head, i);
		pr_indent(indent);
		printf("%ld. ", i);
		lump_dump(key);
		printf(" = %lld\n", block);
		node_dump(node->user, block, indent + 1);
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
	buf_put(node);
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

void store_rec(Head_s *head, Lump_s key, Lump_s val, unint i)
{
FN;
	//lf_compact(head);
	store_lump(head, val);
	store_lump(head, key);
	store_end(head, i);
}

void store_twig(Head_s *head, Twig_s twig, unint i)
{
FN;
	//br_compact(head);
	store_block(head, twig.block);
	store_lump(head, twig.key);
	store_end(head, i);
}

void delete_rec(Head_s *head, unint i)
{
FN;
	Lump_s	key;
	Lump_s	val;

	assert(i < head->num_recs);
	assert(head->kind == LEAF);
	key = get_key(head, i);
	val = get_val(head, i);
	head->free += key.size + val.size + SZ_LEAF_OVERHEAD;
	--head->num_recs;
	if (i < head->num_recs) {
		memmove(&head->rec[i], &head->rec[i+1],
			(head->num_recs - i) * SZ_U16);
	}
}

void lf_audit(const char *fn, unsigned line, Head_s *head)
{
FN;
	int	i;
	int	free = SZ_BUF - SZ_HEAD;

	assert(head->kind == LEAF);
	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;
		Lump_s val;

		key = get_key(head, i);
		val = get_val(head, i);
		free -= key.size + val.size + 3 * SZ_U16;
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
	Lump_s	key;
	Lump_s	val;

	LF_AUDIT(head);
	assert(i < head->num_recs);
	key = get_key(head, i);
	val = get_val(head, i);
	head->free += key.size + val.size + 3 * SZ_U16;

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

	rec.key = get_key(src, j);
	rec.val = get_val(src, j);

	store_rec(dst, rec.key, rec.val, i);
}	

void lf_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Rec_s	rec;

	rec.key = get_key(src, j);
	rec.val = get_val(src, j);

	store_rec(dst, rec.key, rec.val, i);

	lf_del_rec(src, j);
}	

void lf_compact(Buf_s *bleaf)
{
FN;
	Head_s	*head = bleaf->d;
	Buf_s	*b;
	Head_s	*h;
	int	i;

	if (usable(head) == head->free) {
HERE;
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

	twig.key = get_key(src, j);
	twig.block = get_block(src, j);

	store_twig(dst, twig, i);
}	

void br_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Twig_s	twig;

	twig.key = get_key(src, j);
	twig.block = get_block(src, j);
	store_twig(dst, twig, i);

	br_del_rec(src, j);
}	

void br_compact(Buf_s *br)
{
FN;
	Head_s	*head = br->d;
	Buf_s	*b;
	Head_s	*h;
	int	i;

	if (usable(head) == head->free) {
HERE;
		return;
	}
	b = buf_scratch(br->cache);
	h = b->d;
	init_node(b, br->user, BRANCH);
	h->is_split = head->is_split;
	h->block    = head->block;
	h->last     = head->last;
	for (i = 0; i < head->num_recs; i++) {
		br_rec_copy(h, i, head, i);
	}
	memmove(head, h, SZ_BUF);
	buf_toss(b);
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

int lf_insert(Buf_s *bleaf, Lump_s key, Lump_s val)
{
FN;
	Buf_s	*right;
	Head_s	*head = bleaf->d;
	int	size  = key.size + val.size + SZ_LEAF_OVERHEAD;
	int	i;

	LF_AUDIT(head);
	while (size > head->free) {
		right = lf_split(bleaf);
		if (isLE(right->d, key, 0)) {
			buf_put(right);
		} else {
			buf_put(bleaf);
			bleaf = right;
			head = bleaf->d;
		}
	}
	if (size > usable(head)) {
		lf_compact(bleaf);
	}
	i = find_le(head, key);
	store_rec(head, key, val, i);
	LF_AUDIT(head);
	buf_put(bleaf);
	return 0;
}

Buf_s *grow(Buf_s *bchild)
{
FN;
	Btree_s	*t     = bchild->user;
	Head_s	*child = bchild->d;
	Head_s	*parent;
	Buf_s	*bparent;
	Twig_s	twig;

	bparent = br_new(t);
	parent = bparent->d;
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
	t->root = parent->block;
	return bparent;
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
			buf_put(right);
		} else {
			buf_put(bparent);
			bparent = right;
			parent  = bparent->d;
		}
		x = find_le(parent, twig.key);
	}
	if (size > usable(parent)) {
HERE;
		br_compact(bparent);
	}
	twig.block = child->block;
	store_twig(parent, twig, x);
	return bparent;
}

Buf_s *br_store(Buf_s *bparent, Buf_s *bchild, int x)
{
FN;
	Head_s	*parent = bparent->d;
	Head_s	*child  = bchild->d;
	int	size;
	Twig_s	twig;

	twig.key = get_key(child, child->num_recs - 1);
	size = twig.key.size + SZ_U64 + SZ_LEAF_OVERHEAD;

	while (size > parent->free) {
		Buf_s	*right = br_split(bparent);

		if (isLE(parent, twig.key, parent->num_recs - 1)) {
			buf_put(right);
		} else {
			buf_put(bparent);
			bparent = right;
			parent  = bparent->d;
		}
		x = find_le(parent, twig.key);
	}
	if (size > usable(parent)) {
		br_compact(bparent);
	}
	if (x == parent->num_recs) {
		twig.block = parent->last;
		parent->last = child->last;
	} else {
		update_block(parent, child->last, x);
		twig.block = child->block;
	}
	store_twig(parent, twig, x);
	if (child->kind == BRANCH) {
		child->last = get_block(child, child->num_recs - 1);
		br_del_rec(child, child->num_recs - 1);	
	} else {
		child->last = 0;
	}
	child->is_split = FALSE;
	return bparent;
}

int br_insert(Buf_s *bparent, Lump_s key, Lump_s val)
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
			bparent = br_store(bparent, bchild, x);
			parent = bparent->d;
			buf_put(bchild);
			continue;
		}
		if (child->kind == LEAF) {
			buf_put(bparent);
			lf_insert(bchild, key, val);
			return 0;
		}
		buf_put(bparent);
		bparent = bchild;
		parent = bparent->d;
	}
}

int t_insert(Btree_s *t, Lump_s key, Lump_s val)
{
FN;
	Buf_s	*node;
	Head_s	*head;
	int	rc;

	if (t->root) {
		node = buf_get(t->cache, t->root);
	} else {
		node = lf_new(t);
		t->root = node->block;
	}
	head = node->d;
	if (head->is_split) {
		Buf_s *new_node = grow(node);
		buf_put(node);
		node = new_node;
		head = node->d;
	}
	if (head->kind == LEAF) {
		rc = lf_insert(node, key, val);
	} else {
		rc = br_insert(node, key, val);
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
				buf_put(bleaf);
				bleaf = right;
				head = bleaf->d;
			} else {
				return BT_ERR_NOT_FOUND;
			}
		} else {
			v = get_val(head, i);
			*val = duplump(v);
			buf_put(bleaf);
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
			buf_put(bparent);
			lf_find(bchild, key, val);
			return 0;
		}
		buf_put(bparent);
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
				buf_put(bleaf);
				bleaf = right;
				head = bleaf->d;
			} else {
				return BT_ERR_NOT_FOUND;
			}
		} else {
			delete_rec(head, i);
			buf_put(bleaf);
			return 0;
		}
	}
}

Buf_s *rebalance(Buf_s *bparent, int x, u64 block, Buf_s *bchild)
{
	Head_s	*parent = bparent->d;
	Head_s	*child  = bchild->d;
	Buf_s	*bsibling;
	Head_s	*sibling;
	int	y;
	int	i;
PRd(block);
	if (x == parent->num_recs) {
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
	if (sibling->num_recs == 0) {
		//XXX: should delete it
		buf_put(bsibling);
		return bparent;
	}
	if (child->kind == LEAF) {
		lf_compact(bchild);
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
		br_compact(bchild);
		for (i = 0; ;i++) {
			//XXX: see comments above.
			br_rec_move(child, child->num_recs, sibling, 0);
			if (child->free <= sibling->free) break;
		}
	}
	buf_put(bsibling);
pr_branch(bparent->d);
	br_del_rec(parent, x);
pr_branch(bparent->d);
	bparent = br_reinsert(bparent, bchild, x);
pr_branch(bparent->d);
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
			bparent = br_store(bparent, bchild, x);
			parent = bparent->d;
			buf_put(bchild);
			continue;
		} else if (child->free > REBALANCE) {
			bparent = rebalance(bparent, x, block, bchild);
		}
		if (child->kind == LEAF) {
			buf_put(bparent);
			lf_delete(bchild, key);
			return 0;
		}
		buf_put(bparent);
		bparent = bchild;
		parent = bparent->d;
	}
}

int t_delete(Btree_s *t, Lump_s key)
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
	for (i = 0; i < size; i++) {
		if (isprint(start[i])) {
			printf("%c", start[i]);
		} else {
			putchar('.');
		}
	}
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
	for (i = 0; i < size; i++) {
		if (isprint(start[i])) {
			printf("%c", start[i]);
		} else {
			putchar('.');
		}
	}
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
	for (i = 0; i < size; i++) {
		if (isprint(start[i])) {
			printf("%c", start[i]);
		} else {
			putchar('.');
		}
	}
	start += size;
	size = *start++;
	size |= (*start++) << 8;
	if (x + size >= SZ_BUF) {
		printf("val size at %ld offset %ld is too big %ld\n",
			i, x, size);
		return FALSE;
	}
	printf(" : %4ld:", size);
	for (i = 0; i < size; i++) {
		if (isprint(start[i])) {
			printf("%c", start[i]);
		} else {
			putchar('.');
		}
	}
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

bool pr_index (Head_s *head, unint i)
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

	block  = (u64)start[0];
	block |= (u64)start[1] << 1*BITS_U8;
	block |= (u64)start[2] << 2*BITS_U8;
	block |= (u64)start[3] << 3*BITS_U8;
	block |= (u64)start[4] << 4*BITS_U8;
	block |= (u64)start[5] << 5*BITS_U8;
	block |= (u64)start[6] << 6*BITS_U8;
	block |= (u64)start[7] << 7*BITS_U8;
	printf(" : %lld", block);
	putchar('\n');
	return TRUE;
}

void pr_branch(Head_s *head)
{
	unint	i;

	pr_head(head, 0);
	for (i = 0; i < head->num_recs; i++) {
		pr_index(head, i);
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
	unint	i;

	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;
		Lump_s val;

		key = get_key(head, i);
		val = get_val(head, i);
		printf("%4ld. ", Recnum++);
		lump_dump(key);
		printf("\t");
		lump_dump(val);
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
	buf_put(node);
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
	Lump_s	key;
	Lump_s	val;
	int	rc;

	for (i = 0; i < head->num_recs; i++) {
		key = get_key(head, i);
		val = get_val(head, i);
		rc = apply.func(key, val, apply.user);
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
	buf_put(node);
	return rc;
}

int t_map(Btree_s *t, Apply_f func, void *user)
{
	Apply_s	apply = mk_apply(func, user);
	int	rc = node_map(t, t->root, apply);
	cache_balanced(t->cache);
	return rc;
}

static void pr_lump (Lump_s a) {
	enum { MAX_PRINT = 32 };
	int	i;
	int	size;

	size = a.size;
	if (size > MAX_PRINT) size = MAX_PRINT;
	for (i = 0; i < size; i++) {
		if (isprint(a.d[i])) {
			putchar(a.d[i]);
		} else {
			putchar('.');
		}
	}
}
	
static int rec_audit (Lump_s key, Lump_s val, void *user) {
	Lump_s	*old = user;

	if (cmplump(key, *old) < 0) {
		pr_lump(key);
		printf(" < ");
		pr_lump(*old);
		printf("  ");
		fatal("keys out of order");
		return FAILURE;
	}
	freelump(*old);
	*old = duplump(key);
	return 0;
}

static int node_audit(Btree_s *t, u64 block, Audit_s *audit);

static int leaf_audit(Buf_s *node, Audit_s *audit)
{
	Head_s	*head = node->d;

	++audit->leaves;
#if 0
	unint	i;
	Lump_s	key;
	Lump_s	val;
	int	rc;
	for (i = 0; i < head->num_recs; i++) {
		key = get_key(head, i);
		val = get_val(head, i);
		rc = apply.func(key, val, apply.user);
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
	buf_put(node);
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
