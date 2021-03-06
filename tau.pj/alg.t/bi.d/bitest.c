/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>

#include "bitree.h"
#include "lump.h"
#include "main.h"
#include "util.h"

static void pr_lump (Lump_s a)
{
	enum { MAX_PRINT = 32 };
	int i;
	int size;

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

/*static*/ int pr_rec (Rec_s rec, BiNode_s *root, void *user)
{
	u64 *recnum = user;

	printf("%4lld. ", ++*recnum);
	pr_lump(rec.key);
	printf(" = ");
	pr_lump(rec.val);
	printf("\n");
	return 0;
}

static void pr_audit (BiTree_s *tree)
{
	BiAudit_s a = bi_audit(tree);
	printf("num nodes=%lld sqrt=%g log2=%g\n"
		"max depth=%lld\n"
		"avg depth=%g\n"
		"num left =%lld\n"
		"num right=%lld\n",
		a.num_nodes, sqrt(a.num_nodes), log(a.num_nodes)/log(2.0),
		a.max_depth,
		(double)a.total_depth / (double)a.num_nodes,
		a.num_left,
		a.num_right);
	bi_pr_path(tree, a.deepest);
}

void test1(int n)
{
	Lump_s key;
	unint i;

	for (i = 0; i < n; i++) {
		key = seq_lump();
		printf("%s\n", key.d);
		freelump(key);
	}
}

void test_seq(int n)
{
	BiTree_s tree = { 0 };
	Rec_s rec;
	unint i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		rec.key = seq_lump();
		rec.val = rnd_lump();
		bi_insert(&tree, rec);
	}
	bi_print(&tree);
}

void test_rnd(int n)
{
	BiTree_s tree = { 0 };
	Rec_s rec;
	unint i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		rec.key = fixed_lump(7);
		rec.val = rnd_lump();
		bi_insert(&tree, rec);
	}
// bi_print(&tree);
// pr_all_records(&tree);
// pr_tree(&tree);
	bi_audit(&tree);
}

void find_find (Lump_s key, Lump_s val, void *user)
{
	BiTree_s *tree = user;
	Lump_s fval;
	int rc;

	rc = bi_find(tree, key, &fval);
	if (rc) {
		fatal("Didn't find %s : rc=%d", key.d, rc);
	} else if (cmplump(val, fval) != 0) {
		fatal("Val not the same %s!=%s", val.d, fval.d);
	} else {
		printf("%s:%s\n", key.d, fval.d);
	}
	freelump(fval);
}

void test_bi_find(int n)
{
	BiTree_s tree = { 0 };
	Rec_s rec;
	unint i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		rec.key = fixed_lump(7);
		rec.val = rnd_lump();
		r_add(rec);
		bi_insert(&tree, rec);
	}
	r_for_each(find_find, &tree);
	bi_audit(&tree);
}

void delete (Lump_s key, Lump_s val, void *user)
{
	BiTree_s *tree = user;
	int rc;

	rc = bi_delete(tree, key);
	if (rc) {
		fatal("Didn't find %s : rc=%d", key.d, rc);
	}
}

void test_bi_delete(int n)
{
	BiTree_s tree = { 0 };
	Rec_s rec;
	unint i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		rec.key = fixed_lump(7);
		rec.val = rnd_lump();
		r_add(rec);
		bi_insert(&tree, rec);
	}
	r_for_each(delete, &tree);
	bi_audit(&tree);
	pr_audit(&tree);
}

int should_delete(s64 count, s64 level)
{
	enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
	return (random() & MASK) * count / level / RANGE;
}

#if 0
if (i >= 595)
{
	fdebugon();
	Option.debug = TRUE;
	Option.print = TRUE;
printf("=========%d======\n", i);
}
if (i > 596)
{
	fdebugoff();
	Option.debug = FALSE;
	Option.print = FALSE;
}
if (Option.print)
{
	printf("delete\n");
	PRlp(key);
	bi_print(root);
}
#endif

void test_bi_level(int n, int level)
{
	BiTree_s tree = { 0 };
	Rec_s rec;
	s64 count = 0;
	int i;
	int rc;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
if (i % 1000 == 0) fprintf(stderr, ".");
		if (should_delete(count, level)) {
			rec.key = r_delete_rand();
			rc = bi_delete(&tree, rec.key);
			if (rc) fatal("delete key=%s", rec.key.d);
			--count;
		} else {
			rec.key = fixed_lump(7);
			rec.val = rnd_lump();
			r_add(rec);
			rc = bi_insert(&tree, rec);
			if (rc) fatal("bi_insert key=%s val=%s",
					rec.key.d, rec.val.d);
			++count;
		}
		bi_audit(&tree);
	}
printf("\n");
	bi_audit(&tree);
	if (Option.print) bi_print(&tree);
	pr_audit(&tree);
}
