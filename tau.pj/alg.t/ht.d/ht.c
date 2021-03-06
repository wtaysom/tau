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

#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>

#include "crfs.h"
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

typedef struct Apply_s {
	Apply_f	func;
	void	*user;
} Apply_s;

static void lump_dump(Lump_s a);
static void lf_dump(Buf_s *buf, int indent);
static void br_dump(Buf_s *buf, int indent);
static void node_dump(Htree_s *t, Blknum_t blknum, int indent);
static void pr_leaf(Node_s *leaf);
static void pr_branch(Node_s *branch);

bool Dump_buf = FALSE;

static inline Apply_s mk_apply (Apply_f func, void *user)
{
	Apply_s a;

	a.func = func;
	a.user = user;
	return a;
}

void cache_err (Htree_s *t)
{
	CacheStat_s cs = cache_stats();
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

static void pr_n_u8 (const void *mem, unint n)
{
	const u8	*u = mem;
	unint		i;

	printf(" ");
	for (i = 4; n < i; i--) printf("  ");

	while (0 < i--) printf("%.2x", u[i]);
}

static void pr_mem (const void *mem, unsigned n)
{
	enum {	NLONGS = 4,
		NCHARS = NLONGS * sizeof(u32) };

	const u32	*p = mem;
	const char	*c = mem;
	unsigned	i, j;
	unsigned	q, r;

	q = n / NCHARS;
	r = n % NCHARS;
	for (i = 0; i < q; i++) {
		printf("%p:", p);
		for (j = 0; j < NLONGS; j++) {
			printf(" %8x", *p++);
		}
		printf(" | ");
		for (j = 0; j < NCHARS; j++, c++) {
			printf("%c", isprint(*c) ? *c : '.');
		}
		printf("\n");
	}
	if (!r) return;
	printf("%8p:", p);
	for (j = 0; j < r / sizeof(u32); j++) {
		printf(" %8x", *p++);
	}
	i = r % sizeof(u32);
	if (i) {
		++j;
		pr_n_u8(p, i);
	}
	for (; j < NLONGS; j++) {
		printf("         ");
	}
	printf(" | ");
	for (j = 0; j < r; j++, c++) {
		printf("%c", isprint(*c) ? *c : '.');
	}
	printf("\n");
}

static Buf_s *t_get (Htree_s *t, Blknum_t blknum)
{
	Buf_s *buf = buf_get(&t->inode, blknum);
	buf->user = t;
	return buf;
}

Stat_s      t_get_stats   (Htree_s *t) { return t->stat; }

static int usable (Node_s *leaf)
{
FN;
	assert(leaf->isleaf);
	return leaf->end - SZ_HEAD - SZ_U16 * leaf->numrecs;
}

static int inuse (Node_s *leaf)
{
	return BLOCK_SIZE - SZ_HEAD - leaf->free;
}

static Key_t get_key (Node_s *leaf, unint i)
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

static Lump_s get_val (Node_s *leaf, unint i)
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

static Hrec_s get_rec (Node_s *leaf, unint i)
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
	
static void pr_mem_leaf (Node_s *node)
{
	int	i;

	printf("  free %d  end %d\n",
		node->free, node->end);
	for (i = 0; i < node->numrecs; i++) {
		printf(" %d", node->rec[i]);
	}
	printf("\n");
	for (i = 0; i < node->numrecs; i++) {
		Hrec_s	rec = get_rec(node, i);
		printf("%lld %d %s\n",
			(u64)rec.key, rec.val.size,
			(char *)rec.val.d);
	}
}

static void pr_mem_br (Node_s *node)
{
	int	i;

	printf("  first %lld\n", (u64)node->first);
	for (i = 0; i <= node->numrecs && i < NUM_TWIGS; i++) {
		printf(" %8lld %8lld\n", (u64)node->twig[i].key,
			(u64)node->twig[i].blknum);
	}
}

void pr_mem_node (Node_s *node)
{
	printf("isleaf %d  unused1 %d  numrecs %d blknum %lld",
		node->isleaf, node->unused1, node->numrecs,
		(u64)node->blknum);
	if (node->isleaf) pr_mem_leaf(node);
	else pr_mem_br(node);
}

static void pr_indent (int indent)
{
	int i;

	for (i = 0; i < indent; i++) {
		printf("  ");
	}
}

static void pr_head (Node_s *node, int indent)
{
	pr_indent(indent);
	if (node->isleaf) {
		printf("LEAF %lld numrecs=%d free=%d end=%d\n",
			(u64)node->blknum,
			node->numrecs,
			node->free,
			node->end);
	} else {
		printf("BRANCH %lld numrecs=%d first=%lld\n",
			(u64)node->blknum,
			node->numrecs,
			(u64)node->first);
	}
}

static void pr_buf(Buf_s *buf, int indent)
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

static void pr_chars (int n, char *a)
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
	if (MAX_PRINT < size) {
		size = MAX_PRINT;
		pr_chars(MAX_PRINT/2 - 3, a.d);
		printf(" ... ");
		pr_chars(MAX_PRINT/2 - 2, &((char *)a.d)[MAX_PRINT/2 + 2]);
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

static void pr_leaf(Node_s *leaf)
{
	unint	i;
	Hrec_s	rec;

	pr_head(leaf, 0);
	for (i = 0; i < leaf->numrecs; i++) {
		rec = get_rec(leaf, i);
		printf("%3ld. ", i);
		pr_rec(rec);
		putchar('\n');
	}
}

static void pr_branch(Node_s *branch)
{
	unint	i;
	Twig_s	*twig = branch->twig;

	pr_head(branch, 0);
	printf(" first: %llx\n", (u64)branch->first);
	for (i = 0; i < branch->numrecs; i++, twig++) {
		printf("%3ld. %lld : %lld\n",
			i, (u64)twig->key, (u64)twig->blknum);
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

static void init_node (Node_s *node, bool isleaf, Blknum_t blknum)
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

static void key_dump (Key_t key)
{
	printf("%llu", (u64)key);
}

static void lump_dump(Lump_s a)
{
#if 0
	int	i;

	for (i = 0; i < a.size; i++) {
		putchar(a.d[i]);
	}
#else
	printf("%.*s", a.size, (char *)a.d);
#endif
}

static void rec_dump (Hrec_s rec)
{
	key_dump(rec.key);
	printf(" = ");
	lump_dump(rec.val);
	printf("\n");
}

static void lf_dump (Buf_s *buf, int indent)
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

static void br_dump (Buf_s *buf, int indent)
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
		printf(" = %lld\n", (u64)twig.blknum);
		node_dump(buf->user, twig.blknum, indent + 1);
	}
}

static void node_dump (Htree_s *t, Blknum_t blknum, int indent)
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
static bool isGE (Node_s *leaf, Key_t key, unint i)
{
FN;
	Key_t	target;

	target = get_key(leaf, i);
	return key <= target;
}
#endif

static bool isLT (Node_s *leaf, Key_t key, unint i)
{
	Key_t	target;

	target = get_key(leaf, i);
	return key < target;
}

static int isEQ (Node_s *leaf, Key_t key, unint i)
{
	Key_t	target;

	target = get_key(leaf, i);
	if (key < target) return -1;
	if (key == target) return 0;
	return 1;
}

static void store_key (Node_s *leaf, Key_t key)
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

static void store_lump (Node_s *leaf, Lump_s lump)
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

/* XXX: Don't like this name */
static void store_end (Node_s *leaf, unint i)
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

static void rec_copy(Node_s *dst, int a, Node_s *src, int b)
{
	assert(dst->isleaf);
	assert(src->isleaf);

	Hrec_s rec = get_rec(src, b);

	store_lump(dst, rec.val);
	store_key(dst, rec.key);
	store_end(dst, a);
}

static void lf_compact (Node_s *leaf)
{
FN;
	u8	b[BLOCK_SIZE];
	Node_s	*h = (Node_s *)b;
	int	i;

	if (usable(leaf) == leaf->free) {
		return;
	}
	zero(b);
	init_node(h, TRUE, leaf->blknum);
	for (i = 0; i < leaf->numrecs; i++) {
		rec_copy(h, i, leaf, i);
	}
	memmove(leaf, h, BLOCK_SIZE);
}

static void store_rec (Node_s *leaf, Key_t key, Lump_s val, unint i)
{
FN;
	assert(leaf->isleaf);
	store_lump(leaf, val);
	store_key(leaf, key);
	store_end(leaf, i);
}

static void delete_rec (Node_s *leaf, unint i)
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

static void lf_audit (const char *fn, unsigned line, Node_s *leaf)
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

static void br_audit (const char *fn, unsigned line, Node_s *branch)
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

static int leaf_lt (Node_s *leaf, Key_t key)
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

static int br_lt (Node_s *branch, Key_t key)
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

static int leaf_eq (Node_s *leaf, Key_t key)
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
		if (0 == eq) {
			return x;
		} else if (0 < eq) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	if (left == leaf->numrecs) return left;
	return -1;
}

static int leaf_le (Node_s *leaf, Key_t key)
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
		if (0 == eq) {
			return x;
		} else if (0 < eq) {
			left = x + 1;
		} else {
			right = x - 1;
		}
	}
	return left;
}

static void lf_del_rec (Node_s *leaf, unint i)
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

static void lf_rec_move(Node_s *dst, int i, Node_s *src, int j)
{
FN;
	Hrec_s	rec = get_rec(src, j);
	store_rec(dst, rec.key, rec.val, i);
	lf_del_rec(src, j);
}

static void lf_move_recs (Node_s *leaf, Node_s *sibling)
{
	int	i;
	int	j;
	int	size;
	int	total;

	lf_compact(leaf);

	j = leaf->numrecs;
	for (i = 0; i < sibling->numrecs; i++, j++) {
		unint	x = sibling->rec[i];
		u8	*start = &((u8 *)sibling)[x];

		size = start[sizeof(Key_t)];
		size |= start[sizeof(Key_t) + 1] << 8;
		size += sizeof(u16) + sizeof(Key_t);
		total = size + sizeof(u16);
		assert(total <= leaf->free);
		leaf->end -= size;
		leaf->rec[j] = leaf->end;
		leaf->free -= total;
		memmove(&((u8 *)leaf)[leaf->end], start, size);
		++leaf->numrecs;
	}
	sibling->numrecs = 0;
	sibling->free = BLOCK_SIZE - SZ_HEAD;
}

static void br_twig_del (Node_s *branch, unint irec)
{
FN;
	BR_AUDIT(branch);
	assert(irec < branch->numrecs);

	--branch->numrecs;
	if (irec < branch->numrecs) {
		memmove( &branch->twig[irec], &branch->twig[irec + 1],
			(branch->numrecs - irec) * sizeof(Twig_s));
	}
	BR_AUDIT(branch);
}

static Buf_s *node_new (Htree_s *t, u8 isleaf)
{
FN;
	Buf_s	*buf = t->inode.type->new(&t->inode);

	buf->user = t;
	init_node(buf->d, isleaf, buf->blknum);
	return buf;
}

static void node_free (Htree_s *t, Buf_s **bp)
{
	Buf_s	*buf = *bp;
	Node_s	*node = buf->d;

	if (node->isleaf) {
		++t->stat.leaf.deleted;
	} else {
		++t->stat.branch.deleted;
	}
	buf_free(bp);
}

static Buf_s *br_new (Htree_s *t)
{
FN;
	++t->stat.branch.new;
	return node_new(t, FALSE);
}

static Buf_s *lf_new (Htree_s *t)
{
FN;
	++t->stat.leaf.new;
	return node_new(t, TRUE);
}

static int lf_insert (Buf_s *buf, Key_t key, Lump_s val, int size)
{
FN;
	Node_s *leaf = buf->d;
	int i;

	LF_AUDIT(leaf);
	if (usable(leaf) < size) {
		lf_compact(leaf);
	}
	i = leaf_lt(leaf, key);
	store_rec(leaf, key, val, i);
	LF_AUDIT(leaf);
	buf_put_dirty( &buf);
	return 0;
}

static bool is_full (Node_s *node, int size)
{
	if (node->isleaf) {
		return node->free < size;
	} else {
		return node->numrecs == NUM_TWIGS;
	}
}

static bool is_low (Node_s *node)
{
FN;
	if (node->isleaf) {
		return LEAF_SPLIT < node->free;
	} else {
		return node->numrecs < BRANCH_SPLIT;
	}
}

static Buf_s *grow (Htree_s *t, int size)
{
FN;
	Buf_s		*buf;
	Node_s		*node;
	Buf_s		*bparent;
	Node_s		*parent;
	Blknum_t	root;

	root = get_root(t);
	if (!root) {
		buf = lf_new(t);
		set_root(t, buf->blknum);
		return buf;
	}
	buf = t_get(t, root);
	node = buf->d;
	if (!is_full(node, size)) return buf;

	bparent = br_new(t);
	parent = bparent->d;
	parent->first = node->blknum;
	set_root(t, parent->blknum);
	buf_put( &buf);
	return bparent;
}

static void twig_insert (Node_s *branch, Key_t key, Blknum_t blknum, int irec)
{
	assert(!branch->isleaf);
	memmove( &branch->twig[irec + 1], &branch->twig[irec],
		(branch->numrecs - irec) * sizeof(Twig_s));
	branch->twig[irec].key = key;
	branch->twig[irec].blknum = blknum;
	++branch->numrecs;
	BR_AUDIT(branch);
}

static void split_leaf (Htree_s *t, Node_s *parent, Node_s *leaf, int irec)
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
	++t->stat.leaf.split;
}

static void split_branch (Htree_s *t, Node_s *parent, Node_s *branch, int irec)
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
	++t->stat.branch.split;
	BR_AUDIT(branch);
	BR_AUDIT(sibling);
	BR_AUDIT(parent);
	buf_put_dirty( &buf);
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
	cache_balanced();
	++t->stat.insert;
	return rc;
}

static int lf_find(Buf_s *buf, Key_t key, Lump_s *vp)
{
FN;
	Node_s	*node = buf->d;
	int	irec;
	Lump_s	val;

	irec = leaf_eq(node, key);
	if (irec == -1) {
		return HT_ERR_NOT_FOUND;
	}
	if (irec == node->numrecs) {
		return HT_ERR_NOT_FOUND;
	} else {
		if (vp) {
			val = get_val(node, irec);
			*vp = duplump(val);
		}
		return 0;
	}
}

int t_find (Htree_s *t, Key_t key, Lump_s *vp)
{
FN;
	Buf_s		*buf;
	Node_s		*node;
	Buf_s		*bchild;
	Node_s		*child;
	int		irec;
	int		rc;
	Blknum_t	root;

	root = get_root(t);
	if (!root) return HT_ERR_NOT_FOUND;
	buf = t_get(t, root);
	node = buf->d;
	while (!node->isleaf) {
		irec = br_lt(node, key);
		bchild = t_get(t, node->twig[irec - 1].blknum);
		child = bchild->d;
		buf_put( &buf);
		buf = bchild;
		node = child;
	}
	rc = lf_find(buf, key, vp);
	buf_put( &buf);
	cache_balanced();
	if (!rc) ++t->stat.find;
	return rc;
}

static Buf_s *left_most (Htree_s *t, Blknum_t blknum)
{
	Buf_s	*buf;
	Node_s	*node;

	for (;;) {
		buf = t_get(t, blknum);
		node = buf->d;
		if (node->isleaf) return buf;
		blknum = node->first;
		buf_put( &buf);
	}
}

int t_next (Htree_s *t, Key_t prev_key, Key_t *key, Lump_s *vp)
{
FN;
	Buf_s		*buf;
	Node_s		*node;
	Buf_s		*bchild;
	Node_s		*child;
	int		irec;
	int		rc;
	Blknum_t	root;
	Blknum_t	nextrt = 0;

	root = get_root(t);
	if (!root) return HT_ERR_NOT_FOUND;
	buf = t_get(t, root);
	node = buf->d;
	while (!node->isleaf) {
		irec = br_lt(node, prev_key);
		if (irec < node->numrecs) {
			nextrt = node->twig[irec].blknum;
		}
		bchild = t_get(t, node->twig[irec - 1].blknum);
		child = bchild->d;
		buf_put( &buf);
		buf = bchild;
		node = child;
	}
	irec = leaf_le(node, prev_key);
	if (node->numrecs - 1 <= irec) {
		if (nextrt) {
			buf_put(&buf);
			buf = left_most(t, nextrt);
			node = buf->d;
			irec = 0;
		} else {
			rc = HT_ERR_NOT_FOUND;
			goto exit;
		}
	} else {
		Key_t	next_key = get_key(node, irec);
		if (next_key == prev_key) {
			++irec;
		}
	}
	*key = get_key(node, irec);
	if (vp) {
		Lump_s val = get_val(node, irec);
		*vp = duplump(val);
	}
	rc = 0;
exit:
	buf_put( &buf);
	cache_balanced();
	if (!rc) ++t->stat.next;
	return rc;
}

static int node_map(Htree_s *t, Blknum_t blknum, Apply_s apply);

static int lf_map (Buf_s *buf, Apply_s apply)
{
	Node_s	*node = buf->d;
	unint	i;
	Hrec_s	rec;
	int	rc;

	for (i = 0; i < node->numrecs; i++) {
		rec = get_rec(node, i);
		rc = apply.func(rec, apply.user);
		if (rc) return rc;
	}
	return 0;
}

static int br_map (Buf_s *buf, Apply_s apply)
{
	Node_s		*node = buf->d;
	Blknum_t	blknum;
	unint		i;
	int		rc;

	rc = node_map(buf->user, node->first, apply);
	if (rc) return rc;
	for (i = 0; i < node->numrecs; i++) {
		blknum = node->twig[i].blknum;
		rc = node_map(buf->user, blknum, apply);
		if (rc) return rc;
	}
	return 0;
}

static int node_map (Htree_s *t, Blknum_t blknum, Apply_s apply)
{
	Buf_s	*buf;
	Node_s	*node;
	int	rc = 0;

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

int t_map (Htree_s *t, Apply_f func, void *user)
{
	Apply_s	apply = mk_apply(func, user);
	int	rc = node_map(t, get_root(t), apply);

	cache_balanced();
	return rc;
}

static int map_rec_audit (Hrec_s rec, void *user)
{
	Key_t	*oldkey = user;

	if (rec.key < *oldkey) {
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

static int leaf_audit (Buf_s *buf, Audit_s *audit, int depth)
{
	Node_s	*node = buf->d;

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

static int branch_audit (Buf_s *buf, Audit_s *audit, int depth)
{
	Node_s		*node = buf->d;
	Blknum_t	blknum;
	unint		i;
	int		rc;

	++audit->branches;
	rc = node_audit(buf->user, node->first, audit, depth);
	if (rc) return rc;
	for (i = 0; i < node->numrecs; i++) {
		blknum = node->twig[i].blknum;
		rc = node_audit(buf->user, blknum, audit, depth);
		if (rc) return rc;
	}
	return 0;
}

static int node_audit (Htree_s *t, Blknum_t blknum, Audit_s *audit, int depth)
{
	Buf_s	*buf;
	Node_s	*node;
	int	rc = 0;

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
	Key_t	oldkey = 0;
	int	rc;

	zero(*audit);
	rc = t_map(t, map_rec_audit, &oldkey);
	if (rc) {
		printf("AUDIT FAILED %d\n", rc);
		return rc;
	}
	rc = node_audit(t, get_root(t), audit, 0);
	return rc;
}

static int lf_delete (Buf_s *buf, Key_t key)
{
FN;
	Node_s	*node = buf->d;
	int	x;

	x = leaf_eq(node, key);
	if (x == -1) {
		return HT_ERR_NOT_FOUND;
	}
	if (x == node->numrecs) {
		return HT_ERR_NOT_FOUND;
	} else {
		delete_rec(node, x);
		buf_dirty(buf);
		return 0;
	}
}

static bool join_leaf (Htree_s *t, Node_s *parent, Node_s *leaf, int irec)
{
FN;
	Buf_s	*buf;
	Node_s	*sibling;

	buf = t_get(t, parent->twig[irec].blknum);
	sibling = buf->d;
	if (inuse(sibling) < leaf->free) {
		lf_move_recs(leaf, sibling);
		node_free(t, &buf);
		br_twig_del(parent, irec);
		++t->stat.leaf.join;
		return TRUE;
	} else {
		/* Rebalance */
		buf_put( &buf);
		return FALSE;
	}
}

static bool join_branch (Htree_s *t, Node_s *parent, Node_s *branch, int irec)
{
FN;
	Buf_s	*buf;
	Node_s	*sibling;

	BR_AUDIT(branch);
	buf = t_get(t, parent->twig[irec].blknum);
	sibling = buf->d;
	BR_AUDIT(sibling);
	if (branch->numrecs + sibling->numrecs + 1 < NUM_TWIGS) {
		int n = branch->numrecs;
		branch->twig[n].key = parent->twig[irec].key;
		branch->twig[n].blknum = sibling->first;
		memmove( &branch->twig[n + 1], sibling->twig,
			sizeof(Twig_s) * sibling->numrecs);
		branch->numrecs += sibling->numrecs + 1;
		node_free(t, &buf);
		br_twig_del(parent, irec);
		BR_AUDIT(branch);
		++t->stat.branch.join;
		return TRUE;
	} else {
		/* Rebalance */
		buf_put( &buf);
		return FALSE;
	}
}

static bool join (Htree_s *t, Node_s *parent, Node_s *node, int irec)
{
	if (is_low(node)) {
		if (irec == parent->numrecs) {
			/* No right sibling */
			return FALSE;
		}
		if (node->isleaf) {
			return join_leaf(t, parent, node, irec);
		} else {
			return join_branch(t, parent, node, irec);
		}
	} else {
		return FALSE;
	}
}

int t_delete (Htree_s *t, Key_t key)
{
FN;
	Buf_s		*buf;
	Node_s		*node;
	Buf_s		*bchild;
	Node_s		*child;
	int		irec;
	int		rc;
	Blknum_t	root;

	for (;;) {
		root = get_root(t);
		if (!root) return HT_ERR_NOT_FOUND;
		buf = t_get(t, root);
		node = buf->d;
		if (node->numrecs != 0) break;
		if (node->isleaf) {
			set_root(t, 0);
		} else {
			set_root(t, node->first);
		}
		node_free(t, &buf);
	}
	while (!node->isleaf) {
		irec = br_lt(node, key);
		bchild = t_get(t, node->twig[irec - 1].blknum);
		child = bchild->d;
		if (join(t, node, child, irec)) {
			buf_dirty(buf);
			buf_put_dirty( &bchild);
		} else {
			buf_put( &buf);
			buf = bchild;
			node = child;
		}
	}
	rc = lf_delete(buf, key);
	buf_put( &buf);
	cache_balanced();
	if (!rc) ++t->stat.delete;
	return rc;
}

void t_dump (Htree_s *t)
{
	int is_on = fdebug_is_on();

	printf("\n************************t_dump**************************\n");
	if (is_on) fdebugoff();
	node_dump(t, get_root(t), 0);
	if (is_on) fdebugon();
	//cache_balanced();
}

Twig_s CheckTwig[NUM_TWIGS];

Htree_s *t_new (void)
{
FN;
	Htree_s *t;

	t = ezalloc(sizeof(*t));
PRd(BLOCK_SIZE);
PRd(NUM_TWIGS);
PRd(LEAF_SPLIT);
PRd(sizeof(Twig_s));
PRd(sizeof(Node_s));
PRd(sizeof(CheckTwig));
	return t;
}

static int node_compare(Htree_s *a, Blknum_t a_blk,
			Htree_s *b, Blknum_t b_blk);

static int leaf_compare (Node_s *a_node, Node_s *b_node)
{
	Hrec_s	a_rec;
	Hrec_s	b_rec;
	int	i;

	if (a_node->free != b_node->free) {
		warn("free differ a=%d b=%d",
			a_node->free, b_node->free);
		return HT_ERR_DIFFERENT;
	}
	if (a_node->end != b_node->end) {
		warn("end differ a=%d b=%d",
			a_node->end, b_node->end);
		return HT_ERR_DIFFERENT;
	}
	for (i = 0; i < a_node->numrecs; i++) {
		a_rec = get_rec(a_node, i);
		b_rec = get_rec(b_node, i);
		if (a_rec.key != b_rec.key) {
			warn("keys differ a=%lld b=%lld",
				(u64)a_rec.key, (u64)b_rec.key);
			return HT_ERR_DIFFERENT;
		}
		if (a_rec.val.size != b_rec.val.size) {
			warn("val size differ a=%d b=%d",
				a_rec.val.size, b_rec.val.size);
			return HT_ERR_DIFFERENT;
		}
		if (memcmp(a_rec.val.d, b_rec.val.d, a_rec.val.size)) {
			warn("val d differ a=%s b=%s",
				(char *)a_rec.val.d, (char *)b_rec.val.d);
			return HT_ERR_DIFFERENT;
		}
	}
	return 0;
}

static int br_compare (Htree_s *a, Node_s *a_node,
		       Htree_s *b, Node_s *b_node)
{
	int	i;
	int	rc;

	rc = node_compare(a, a_node->first,
			  b, b_node->first);
	for (i = 0; i < a_node->numrecs; i++) {
		if (rc) return rc;
		rc = node_compare(a, a_node->twig[i].blknum,
				  b, b_node->twig[i].blknum);
	}
	return rc;
}

static int node_compare (Htree_s *a, Blknum_t a_blk,
			 Htree_s *b, Blknum_t b_blk)
{
	Buf_s	*a_buf = t_get(a, a_blk);
	Buf_s	*b_buf = t_get(b, b_blk);
	Node_s	*a_node;
	Node_s	*b_node;
	int	rc = 0;

	if (a_blk != b_blk) {
		warn("blknum differ a=%lld b=%lld",
			(u64)a_blk, (u64)b_blk);
	}
	if (a_buf == b_buf) return 0;
	if (!a_buf) {
		warn("a_buf NULL %lld", (u64)a_blk);
		rc = HT_ERR_DIFFERENT;
		goto exit;
	}
	if (!b_buf) {
		warn("b_buf NULL %lld", (u64)b_blk);
		rc = HT_ERR_DIFFERENT;
		goto exit;
	}
	a_node = a_buf->d;
	b_node = b_buf->d;
	if (memcmp(a_node, b_node, BLOCK_SIZE) != 0) {
		warn("a %lld and b %lld are not binary identical",
			(u64)a_blk, (u64)b_blk);
		printf("a\n");
		pr_mem(a_node, BLOCK_SIZE);
//		pr_mem_node(a_node);
		printf("b\n");
		pr_mem(b_node, BLOCK_SIZE);
//		pr_mem_node(b_node);
		rc = 0;
		fatal("nodes differ");
	}
	if (a_node->isleaf != b_node->isleaf) {
		warn("Not both leaves a=%d b=%d",
			a_node->isleaf, b_node->isleaf);
		rc = HT_ERR_DIFFERENT;
	}
	if (a_node->unused1 != b_node->unused1) {
		warn("unused differ a=%d b=%d",
			a_node->unused1, b_node->unused1);
		rc = HT_ERR_DIFFERENT;
	}
	if (a_node->numrecs != b_node->numrecs) {
		warn("numrecs differ a=%d b=%d",
			a_node->numrecs, b_node->numrecs);
		rc = HT_ERR_DIFFERENT;
	}
	if (a_node->blknum != b_node->blknum) {
		warn("blknum differ a=%lld b=%lld",
			(u64)a_node->blknum, (u64)b_node->blknum);
		rc = HT_ERR_DIFFERENT;
	}
	if (rc) goto exit;
	
	if (a_node->isleaf) {
		rc = leaf_compare(a_node, b_node);
	} else {
		rc = br_compare(a, a_node, b, b_node);
	}
exit:
	if (a_buf) buf_put( &a_buf);
	if (b_buf) buf_put( &b_buf);
	return rc;	
}

int t_compare (Htree_s *a, Htree_s *b)
{
	int	rc;
	Blknum_t	a_root = get_root(a);
	Blknum_t	b_root = get_root(b);

	if (a_root != b_root) {
		warn("root block nums differ %lld %lld",
			(u64)a_root, (u64)b_root);
		return HT_ERR_DIFFERENT;
	}
	if (!a_root) return 0;

	rc = node_compare(a, a_root, b, b_root);

	cache_balanced();
	return rc;
}

#if 0
Htree_s *t_new (char *file, int num_bufs)
{
	warn("Not Implmented");
	return 0;
}

void t_dump (Htree_s *t)
{
	warn("Not Implmented");
}

int  t_insert (Htree_s *t, Key_t key, Lump_s val)
{
	warn("Not Implmented");
	return 0;
}

int  t_delete (Htree_s *t, Key_t key)
{
	warn("Not Implmented");
	return 0;
}

int t_audit (Htree_s *t, Audit_s *audit)
{
	warn("Not Implmented");
	return 0;
}

int  t_map   (Htree_s *t, Apply_f func, void *user)
{
	warn("Not Implmented");
	return 0;
}

int t_find (Htree_s *t, Key_t key, Lump_s *val)
{
	warn("Not Implmented");
	return 0;
}
#endif
