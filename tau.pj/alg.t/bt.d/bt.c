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
	u8 kind;
	u16 num_recs;
	u16 available;
	u16 end;
	u64 block;
	u64 overflow;
	u64 last;
	u16 rec[];
} Head_s;

struct Btree_s {
	Cache_s *cache;
	u64 root;
};

enum {	MAX_U16 = (1 << 16) - 1,
	BITS_U8 = 8,
	MASK_U8 = (1 << BITS_U8) - 1,
	SZ_BUF = 256,
	SZ_U16 = sizeof(u16),
	SZ_U64 = sizeof(u64),
	SZ_HEAD = sizeof(Head_s),
	SZ_LEAF_OVERHEAD = 3 * SZ_U16,
	SZ_BRANCH_OVERHEAD = 2 * SZ_U16 };

const Lump_s Nil = { -1, NULL };

int lumpcmp(Lump_s a, Lump_s b)
{
FN;
	int size;
	int x;

	if (a.size > b.size) {
		size = b.size;
	} else {
		size = a.size;
	}
	x = memcmp(a.d, b.d, size);
	if (x) return x;
	if (a.size < b.size) return -1;
	if (a.size == b.size) return 0;
	return 1;
}

Lump_s lumpdup(Lump_s a)
{
FN;
	Lump_s	b;

	b.d = malloc(a.size);
	b.size = a.size;
	memmove(b.d, a.d, a.size);
	return b;
}

void lumpfree(Lump_s a)
{
	if (a.d) free(a.d);
}

#if 0
static int node_free(Head_s *head)
{
	return head->end - (SZ_HEAD + head->num_recs * SZ_U16);
}
#endif

static void pr_indent(int indent)
{
	int i;

	for (i = 0; i < indent; i++) {
		printf("  ");
	}
}

static void pr_head(Head_s *head, int indent)
{
	pr_indent(indent);
	printf("num_recs %d "
		"available %d "
		"end %d "
		"block %lld "
		"overflow %lld\n",
		head->num_recs,
		head->available,
		head->end,
		head->block,
		head->overflow);
}

static void pr_buf(Buf_s *buf, int indent)
{
	unint i, j;
	u8 *d = buf->d;
	char *c = buf->d;

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

static Lump_s get_key(Head_s *head, unint i)
{
	Lump_s key;
	unint x;
	u8 *start;

	assert(i < head->num_recs);
	x = head->rec[i];
	assert(x < SZ_BUF);
	start = &((u8 *)head)[x];
	key.size = *start++;
	key.size |= (*start++) << 8;
	key.d = start;
	return key;
}

static Lump_s get_val(Head_s *head, unint i)
{
	Lump_s val;
	unint x;
	u8 *start;
	unint key_size;

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

static u64 get_block(Head_s *head, unint i)
{
	u64 block;
	unint x;
	u8 *start;
	unint key_size;

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

#if 0
static bool isGE(Head_s *head, Lump_s key, unint i)
{
	Lump_s target;
FN;
	target = get_key(head, i);
	return lumpcmp(key, target) >= 0;	
}
#endif

static bool isLE(Head_s *head, Lump_s key, unint i)
{
	Lump_s target;
FN;
	target = get_key(head, i);
	return lumpcmp(key, target) <= 0;	
}

static void store_lump(Head_s *head, Lump_s lump)
{
	int total = lump.size + SZ_U16;
	u8 *start;

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

static void store_block(Head_s *head, u64 block)
{
	int total = SZ_U64;
	u8 *start;

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

static void store_end(Head_s *head, unint i)
{
	assert(i <= head->num_recs);
	if (i < head->num_recs) {
		memmove(&head->rec[i+1], &head->rec[i],
			(head->num_recs - i) * SZ_U16);
	}
	++head->num_recs;
	head->rec[i] = head->end;
	head->available -= SZ_U16;
}

static void lf_audit(const char *fn, unsigned line, Head_s *head)
{
	int i;
	int avail = SZ_BUF - SZ_HEAD;

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

#if 0
static void br_audit(const char *fn, unsigned line, Head_s *head)
{
	int i;
	int avail = SZ_BUF - SZ_HEAD;

	assert(head->kind == LEAF);
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
#endif

static int find_le(Head_s *head, Lump_s key)
{
FN;
	int x, left, right;

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
static int find_ge(Head_s *head, Lump_s key)
{
FN;
	int x, left, right;

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
static int find_eq(Head_s *head, Lump_s key)
{
FN;
	int x, left, right;

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

static void lf_del_rec(Head_s *head, unint i)
{
	Lump_s key;
	Lump_s val;

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

static void lf_rec_copy(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Lump_s key;
	Lump_s val;

	key = get_key(src, j);
	val = get_val(src, j);

	store_lump(dst, val);
	store_lump(dst, key);
	store_end(dst, i);
}	

static void lf_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Lump_s key;
	Lump_s val;

	key = get_key(src, j);
	val = get_val(src, j);

	store_lump(dst, val);
	store_lump(dst, key);
	store_end(dst, i);

	lf_del_rec(src, j);
}	

#if 0
static void br_del_rec(Head_s *head, unint i)
{
	Lump_s key;

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

static void br_rec_copy(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Lump_s key;
	u64 block;

	key = get_key(src, j);
	block = get_block(src, j);

	store_block(dst, block);
	store_lump(dst, key);
	store_end(dst, i);
}	

static void br_rec_move(Head_s *dst, int i, Head_s *src, int j)
{
FN;
	Lump_s key;
	u64 block;

	key = get_key(src, j);
	block = get_block(src, j);

	store_block(dst, block);
	store_lump(dst, key);
	store_end(dst, i);

	br_del_rec(src, j);
}	
#endif

static void lf_compact(Buf_s *lf)
{
FN;
	Head_s *head = lf->d;
	Buf_s  *b = buf_scratch(lf->cache);
	Head_s *h = b->d;
	int i;

	h->kind = LEAF;
	h->available = SZ_BUF - SZ_HEAD;
	h->end = SZ_BUF;
	h->block = head->block;
	h->overflow = head->overflow;
	h->last = head->last;
	for (i = 0; i < head->num_recs; i++) {
		lf_rec_copy(h, i, head, i);
	}
	memmove(head, h, SZ_BUF);
	buf_put(b);
}

static Buf_s *br_new(Btree_s *t)
{
FN;
	Buf_s *br;
	Head_s *head;

	br = buf_new(t->cache);
	br->user = t;
	head = br->d;
	head->kind = BRANCH;
	head->available = SZ_BUF - SZ_HEAD;
	head->end = SZ_BUF;
	head->block = br->block;
	head->overflow = 0;
	head->last = 0;
	return br;
}

static Buf_s *lf_new(Btree_s *t)
{
FN;
	Buf_s *lf;
	Head_s *head;

	lf = buf_new(t->cache);
	lf->user = t;
	head = lf->d;
	head->kind = LEAF;
	head->available = SZ_BUF - SZ_HEAD;
	head->end = SZ_BUF;
	head->block = lf->block;
	head->overflow = 0;
	head->last = 0;
	return lf;
}

static Buf_s *lf_split(Buf_s *left)
{
FN;
	Head_s *child = left->d;
	Btree_s *t  = left->user;
	Buf_s *right = lf_new(t);
	Head_s *sibling = right->d;
	int middle = child->num_recs / 2;
	int i;

	LF_AUDIT(child);
	for (i = 0; middle < child->num_recs; i++) {
		lf_rec_move(sibling, i, child, middle);
	}
	child->overflow = right->block;
	lf_compact(left);
	LF_AUDIT(child);
	LF_AUDIT(sibling);
	return right;
}

static int lf_insert(Buf_s *lf, Lump_s key, Lump_s val)
{
	Head_s *head = lf->d;
	Buf_s *right;
	int size = key.size + val.size + SZ_LEAF_OVERHEAD;
	int i;

	LF_AUDIT(head);
	if (size > head->available) {
		right = lf_split(lf);
		if (!isLE(right->d, key, 0)) {
			lf = right;
			head = lf->d;
		}
	}
	i = find_le(head, key);
	store_lump(head, val);
	store_lump(head, key);
	store_end(head, i);
	LF_AUDIT(head);
	return 0;
}

static Buf_s *lf_grow(Buf_s *lf)
{
	Btree_s *t = lf->user;
	Head_s *child = lf->d;
	Head_s *parent;
	Buf_s *br;
	Lump_s key;

	assert(child->overflow);

	br = br_new(t);
	parent = br->d;
	parent->last = child->overflow;
	child->overflow = 0;

	key = get_key(child, child->num_recs - 1);
	store_block(parent, child->block);
	store_lump(parent, key);
	store_end(parent, 0);

	t->root = parent->block;
	return br;
}	

static Buf_s *br_grow(Buf_s *lf)
{
	Btree_s *t = lf->user;
	Head_s *head = lf->d;
	Head_s *br_head;
	Buf_s *br;
	Lump_s key;

	assert(head->overflow);

	br = br_new(t);
	br_head = br->d;
	br_head->last = head->overflow;
	head->overflow = 0;
	key = get_key(head, head->num_recs - 1);

	store_block(br_head, head->block);
	store_lump(br_head, key);
	store_end(br_head, 0);

	t->root = br_head->block;
	return br;
}

#if 0
static Buf_s *br_split(Buf_s *left)
{
FN;
	Head_s *head = left->d;
	Btree_s *t  = left->user;
	Buf_s *right = lf_new(t);
	int middle = head->num_recs / 2;
	int i;

	BR_AUDIT(head);
	for (i = 0; middle < head->num_recs; i++) {
		br_rec_move(right->d, i, head, middle);
	}
	head->num_recs = middle;
	head->overflow = right->block;
	BR_AUDIT(head);
	return right;
}
#endif

static void br_store(Buf_s *br, Buf_s *bchild, int x)
{
FN;
	Head_s *parent = br->d;
	Head_s *child = bchild->d;
	Head_s *sibling;
	Buf_s *bsibling;
	Lump_s key;

// Need to make sure we have room, if not split branch

	bsibling = buf_get(br->cache, child->overflow);
	sibling = bsibling->d;
	key = get_key(sibling, 0);

	assert(child->overflow == sibling->block);
	store_block(parent, child->overflow);
	child->overflow = 0;
	store_lump(parent, key);
	if (x == parent->num_recs) {
		store_end(parent, x);
		parent->last = sibling->block;
	} else {
		store_end(parent, x);
	}
}

static int br_insert(Buf_s *br, Lump_s key, Lump_s val)
{
	Head_s *head = br->d;
	Buf_s *node;
	Head_s *child;
	u64 block;
	int x;

	for(;;) {
		x = find_le(head, key);
		if (x == head->num_recs) {
			block = head->last;
		} else {
			block = get_block(head, x);
		}
		node = buf_get(br->cache, block);
		child = node->d;
		if (child->overflow) {
			br_store(br, node, x);
			fatal("Child split. Implement");
		}
		if (child->kind == LEAF) {
			lf_insert(node, key, val);
			return 0;
		}
		br = node;
		head = br->d;
	}
}

Lump_s t_find(Btree_s *t, Lump_s key)
{
	if (t->root == 0) {
		fatal("btree empty");
	}
	return Nil;
}

int t_insert(Btree_s *t, Lump_s key, Lump_s val)
{
	Buf_s *node;
	Head_s *head;
	int rc;
FN;
	if (t->root == 0) {
		node = lf_new(t);
		if (!node) fatal("root new");
		t->root = node->block;
	} else {
		node = buf_get(t->cache, t->root);
		if (!node) fatal("root %lld:", t->root);
	}
	head = node->d;
	if (head->kind == LEAF) {
		if (head->overflow) {
			node = lf_grow(node);
			head = node->d;
		} else {
			rc = lf_insert(node, key, val);
			goto exit;
		}
	}
	assert(head->kind == BRANCH);
	if (head->overflow) {
		node = br_grow(node);
		head = node->d;
	}
	rc = br_insert(node, key, val);
exit:
	return rc;
}

Btree_s *t_new(char *file, int num_bufs) {
	Btree_s *t;
FN;
	t = ezalloc(sizeof(*t));
	t->cache = cache_new(file, num_bufs, SZ_BUF);
	return t;
}

static void node_dump(Btree_s *t, u64 block, int indent);

static void lump_dump(Lump_s a)
{
#if 0
	int i;
	for (i = 0; i < a.size; i++) {
		putchar(a.d[i]);
	}
#else
	printf("%*s", a.size - 1, a.d);
#endif
}

static void lf_dump(Buf_s *node, int indent)
{
	Head_s *head = node->d;
	unint i;

	pr_buf(node, indent);
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
	if (head->overflow) {
		pr_indent(i);
		printf("Leaf Overflow:\n");
		node_dump(node->user, head->overflow, indent + 1);
	}
}

static void br_dump(Buf_s *node, int indent)
{
	Head_s *head = node->d;
	unint i;

	for (i = 0; i < head->num_recs; i++) {
		Lump_s key;
		u64 block;

		key   = get_key(head, i);
		block = get_block(head, i);
		pr_indent(indent);
		lump_dump(key);
		printf(" = %llx\n", block);
		node_dump(node->user, block, indent + 1);
	}
	node_dump(node->user, head->last, indent + 1);
	if (head->overflow) {
		pr_indent(i);
		printf("Branch Overflow:\n");
		node_dump(node->user, head->overflow, indent + 1);
	}
}

static void node_dump(Btree_s *t, u64 block, int indent)
{
	Buf_s *node;
	Head_s *head;

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
}

void t_dump(Btree_s *t)
{
	node_dump(t, t->root, 0);
}
