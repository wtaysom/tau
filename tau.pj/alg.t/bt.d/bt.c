/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <mylib.h>
#include <style.h>

#include "bt.h"
#include "buf.h"

#define LF_AUDIT(_h)	lf_audit(FN_ARG, _h)
#define BR_AUDIT(_h)	br_audit(FN_ARG, _h)

enum { LEAF = 1, BRANCH };

typedef struct Head_s {
	u8	kind;
	u8	is_split;
	u16	num_recs;
	u16	available;
	u16	end;
	u64	block;
	u64	last;
	u64	unused;	// XXX: delete when working
	u16	rec[];
} Head_s;

struct Btree_s {
	Cache_s *cache;
	u64 root;
};

enum {	MAX_U16 = (1 << 16) - 1,
	BITS_U8 = 8,
	MASK_U8 = (1 << BITS_U8) - 1,
	SZ_BUF  = 128,
	SZ_U16  = sizeof(u16),
	SZ_U64  = sizeof(u64),
	SZ_HEAD = sizeof(Head_s),
	SZ_LEAF_OVERHEAD   = 3 * SZ_U16,
	SZ_BRANCH_OVERHEAD = 2 * SZ_U16 };

bool Dump_buf = FALSE;

int allocatable(Head_s *head)
{
FN;
	return head->end - SZ_HEAD - SZ_U16 * head->num_recs;
}

void pr_indent(int indent)
{
FN;
	int	i;

	for (i = 0; i < indent; i++) {
		printf("  ");
	}
}

void pr_head(Head_s *head, int indent)
{
FN;
	pr_indent(indent);
	printf("%s "
		"%s"
		"%lld num_recs %d "
		"available %d "
		"end %d "
		"last %lld\n",
		head->kind == LEAF ? "LEAF" : "BRANCH",
		head->is_split ? "*split* " : "",
		head->block,
		head->num_recs,
		head->available,
		head->end,
		head->last);
}

void pr_buf(Buf_s *buf, int indent)
{
FN;
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
	head->kind      = kind;
	head->is_split  = FALSE;
	head->available = SZ_BUF - SZ_HEAD;
	head->end       = SZ_BUF;
	head->block     = node->block;
	head->last      = 0;
}

Lump_s get_key (Head_s *head, unint i)
{
	Lump_s	key;
	unint	x;
	u8	*start;

	if (i >= head->num_recs) show_stackframe();
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

u64 get_block(Head_s *head, unint i)
{
FN;
	u64	block;
	unint	x;
	u8	*start;
	unint	key_size;

	assert(head->kind == BRANCH);
	assert(i < head->num_recs);
	x = head->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	key_size = *start++;
	key_size |= (*start++) << 8;
	start += key_size;
	block  = (u64)start[0];
	block |= (u64)start[1] << 1*BITS_U8;
	block |= (u64)start[2] << 2*BITS_U8;
	block |= (u64)start[3] << 3*BITS_U8;
	block |= (u64)start[4] << 4*BITS_U8;
	block |= (u64)start[5] << 5*BITS_U8;
	block |= (u64)start[6] << 6*BITS_U8;
	block |= (u64)start[7] << 7*BITS_U8;
	return block;
}

void node_dump(Btree_s *t, u64 block, int indent);

void lump_dump(Lump_s a)
{
FN;
#if 0
	int	i;

	for (i = 0; i < a.size; i++) {
		putchar(a.d[i]);
	}
#else
	printf("%*s", a.size - 1, a.d);
#endif
}

void lf_dump(Buf_s *node, int indent)
{
FN;
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
		pr_indent(i);
		printf("Leaf Split:\n");
		node_dump(node->user, head->last, indent);
	}
}

void br_dump(Buf_s *node, int indent)
{
FN;
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
		printf(" = %llx\n", block);
		node_dump(node->user, block, indent + 1);
	}
	if (head->is_split) {
		pr_indent(i);
		printf("Branch Split:\n");
		node_dump(node->user, head->last, indent);
	} else {
		pr_indent(indent);
		printf("%ld. <last> = %llx\n", i, head->last);
		node_dump(node->user, head->last, indent + 1);
	}
}

void node_dump(Btree_s *t, u64 block, int indent)
{
FN;
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

void store_lump(Head_s *head, Lump_s lump)
{
FN;
	int	total = lump.size + SZ_U16;
	u8	*start;

	assert(total <= head->available);
	assert(total <= head->end);
	assert(lump.size < MAX_U16);

	head->end -= total;
	head->available -= total;
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

	assert(total <= head->available);
	assert(total <= head->end);

	head->end -= total;
	head->available -= total;
	start = &((u8 *)head)[head->end];
	*start++ = block & MASK_U8;
	*start++ = (block >> 1*BITS_U8) & MASK_U8;
	*start++ = (block >> 2*BITS_U8) & MASK_U8;
	*start++ = (block >> 3*BITS_U8) & MASK_U8;
	*start++ = (block >> 4*BITS_U8) & MASK_U8;
	*start++ = (block >> 5*BITS_U8) & MASK_U8;
	*start++ = (block >> 6*BITS_U8) & MASK_U8;
	*start++ = (block >> 7*BITS_U8) & MASK_U8;
}

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
	head->available -= SZ_U16;
}

void lf_audit(const char *fn, unsigned line, Head_s *head)
{
FN;
	int	i;
	int	avail = SZ_BUF - SZ_HEAD;

	assert(head->kind == LEAF);
	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;
		Lump_s val;

		key = get_key(head, i);
		val = get_val(head, i);
		avail -= key.size + val.size + 3 * SZ_U16;
	}
	if (avail != head->available) {
		printf("%s<%d> avail:%d != %d:head->available\n",
			fn, line, avail, head->available);
		exit(2);
	}
}

void br_audit(const char *fn, unsigned line, Head_s *head)
{
FN;
	int	i;
	int	avail = SZ_BUF - SZ_HEAD;

	assert(head->kind == BRANCH);
	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;

		key = get_key(head, i);
		avail -= key.size + SZ_U64 + 2 * SZ_U16;
	}
	if (avail != head->available) {
		printf("%s<%d> avail:%d != %d:head->available\n",
			fn, line, avail, head->available);
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

#if 0
int find_eq(Head_s *head, Lump_s key)
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

void lf_del_rec(Head_s *head, unint i)
{
FN;
	Lump_s	key;
	Lump_s	val;

	LF_AUDIT(head);
	assert(i < head->num_recs);
	key = get_key(head, i);
	val = get_val(head, i);
	head->available += key.size + val.size + 3 * SZ_U16;

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
	Lump_s	key;
	Lump_s	val;

	key = get_key(src, j);
	val = get_val(src, j);

	store_lump(dst, val);
	store_lump(dst, key);
	store_end(dst, i);
}	

void lf_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Lump_s	key;
	Lump_s	val;

	key = get_key(src, j);
	val = get_val(src, j);

	store_lump(dst, val);
	store_lump(dst, key);
	store_end(dst, i);

	lf_del_rec(src, j);
}	

void lf_compact(Buf_s *lf)
{
FN;
	Head_s	*head = lf->d;
	Buf_s	*b = buf_scratch(lf->cache);
	Head_s	*h = b->d;
	int	i;

	init_node(b, lf->user, LEAF);
	h->is_split  = head->is_split;
	h->block     = head->block;
	h->last      = head->last;
	for (i = 0; i < head->num_recs; i++) {
		lf_rec_copy(h, i, head, i);
	}
	memmove(head, h, SZ_BUF);
	buf_put(b);
}

void br_del_rec(Head_s *head, unint i)
{
FN;
	Lump_s	key;

	BR_AUDIT(head);
	assert(i < head->num_recs);
	key = get_key(head, i);
	head->available += key.size + SZ_U64 + 2 * SZ_U16;

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
	Lump_s	key;
	u64	block;

	key = get_key(src, j);
	block = get_block(src, j);

	store_block(dst, block);
	store_lump(dst, key);
	store_end(dst, i);
}	

void br_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Lump_s	key;
	u64	block;

	key = get_key(src, j);
	block = get_block(src, j);

	store_block(dst, block);
	store_lump(dst, key);
	store_end(dst, i);

	br_del_rec(src, j);
}	

void br_compact(Buf_s *br)
{
FN;
	Head_s	*head = br->d;
	Buf_s	*b = buf_scratch(br->cache);
	Head_s	*h = b->d;
	int	i;

	init_node(b, br->user, BRANCH);
	h->is_split = head->is_split;
	h->block    = head->block;
	h->last     = head->last;
	for (i = 0; i < head->num_recs; i++) {
		br_rec_copy(h, i, head, i);
	}
	memmove(head, h, SZ_BUF);
	buf_put(b);
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
	return node_new(t, BRANCH);
}

Buf_s *lf_new(Btree_s *t)
{
FN;
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
	return bsibling;
}

int lf_insert(Buf_s *lf, Lump_s key, Lump_s val)
{
FN;
	Buf_s	*right;
	Head_s	*head = lf->d;
	int	size  = key.size + val.size + SZ_LEAF_OVERHEAD;
	int	i;

	LF_AUDIT(head);
	while (size > head->available) {
		right = lf_split(lf);
		if (isLE(right->d, key, 0)) {
			buf_put(right);
		} else {
// Because we put lf away here, we will put it again
// in the caller and never put away right.
			buf_put(lf);
			lf = right;
			head = lf->d;
		}
	}
	if (size > allocatable(head)) {
		lf_compact(lf);
	}
	i = find_le(head, key);
	store_lump(head, val);
	store_lump(head, key);
	store_end(head, i);
	LF_AUDIT(head);
	return 0;
}

Buf_s *grow(Buf_s *bchild)
{
FN;
	Btree_s	*t     = bchild->user;
	Head_s	*child = bchild->d;
	Head_s	*parent;
	Buf_s	*bparent;
	Lump_s	key;

	bparent = br_new(t);
	parent = bparent->d;
	parent->last = child->last;

	key = get_key(child, child->num_recs - 1);
	store_block(parent, child->block);
	store_lump(parent, key);
	store_end(parent, 0);

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
	return bsibling;
}

void br_store(Buf_s *bfather, Buf_s *bchild, int x)
{
FN;
	Head_s	*father = bfather->d;
	Head_s	*child  = bchild->d;
	int	size;
	Lump_s	key;

	key = get_key(child, child->num_recs - 1);
	size = key.size + SZ_U64 + SZ_LEAF_OVERHEAD;

	while (size > father->available) {
		Buf_s	*bmother = br_split(bfather);

		if (!isLE(father, key, father->num_recs - 1)) {
			bfather = bmother;
			father  = bfather->d;
		}
		x = find_le(father, key);
	}
	if (size > allocatable(father)) {
		br_compact(bfather);
	}
	store_block(father, father->last);
	store_lump(father, key);
	store_end(father, x);
	father->last = child->last;
	if (child->kind == LEAF) {
		child->last = 0;
	} else {
		child->last = get_block(child, child->num_recs - 1);
		br_del_rec(child, child->num_recs - 1);	
	}
	child->is_split = FALSE;
}

int br_insert(Buf_s *br, Lump_s key, Lump_s val)
{
FN;
	Head_s	*parent = br->d;
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
		bchild = buf_get(br->cache, block);
		child = bchild->d;
		if (child->is_split) {
			br_store(br, bchild, x);
			buf_put(bchild);
			continue;	/* Try again */
		}
		if (child->kind == LEAF) {
			lf_insert(bchild, key, val);
			return 0;
		}
		buf_put(br);
		br = bchild;
		parent = br->d;
	}
}

Lump_s t_find(Btree_s *t, Lump_s key)
{
FN;
	if (t->root == 0) {
		fatal("btree empty");
		return Nil;
	}
	return Nil;
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
	buf_put(node);
	cache_balanced(t->cache);
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
FN;
	node_dump(t, t->root, 0);
	cache_balanced(t->cache);
}
