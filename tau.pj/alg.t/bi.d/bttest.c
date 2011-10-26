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

#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>
#include <timer.h>

#include "bttree.h"
#include "main.h"
#include "util.h"

static void pr_audit (BtTree_s *tree)
{
	BtAudit_s a = bt_audit(tree);
	printf(	"num recs    =%lld sqrt=%g log2=%g\n"
		"depth       =%lld\n"
		"num braches =%lld\n"
		"num leaves  =%lld\n"
		"num br keys =%lld\n"
		"recs/leaf   =%g\n",
		a.num_lf_keys, sqrt(a.num_lf_keys), log(a.num_lf_keys)/log(2.0),
		a.depth,
		a.num_branches,
		a.num_leaves,
		a.num_br_keys,
		(double)a.num_lf_keys / (double)a.num_leaves);
	printf("stats:\n"
		"num insertes=%lld\n"
		"num deletes =%lld\n"
		"num recs    =%lld\n",
		tree->stat.num_inserts,
		tree->stat.num_deletes,
		tree->stat.num_inserts - tree->stat.num_deletes);
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

#if 0
static void pr_next (BtTree_s *tree)
{
	u64 key = 0;
	printf("------\n");
	for (;;) {
		key = bt_next(tree, key);
		if (key == 0) return;
		printf("%llu\n", key);
	}
}
#endif

void test_bt (int n)
{
	BtTree_s tree = { 0 };
	u64 key;
	int i;

	k_seed(1);
	for (i = 0; i < n; i++) {
		key = k_rand_key();
		bt_insert(&tree, key);
	}
//	bt_audit(&tree);
	bt_print(&tree);
//	pr_next(&tree);
	pr_audit(&tree);
}

void test_bt_level (int n, int level)
{
	BtTree_s tree = { 0 };
	u64 key;
	s64 count = 0;
	int i;
	int rc;
	u64 start, finish, total;

	k_seed(1);
	k_init();
	start = nsecs();
	for (i = 0; i < n; i++) {
		if (k_should_delete(count, level)) {
			key = k_delete_rand();
			rc = bt_delete(&tree, key);
			if (rc) fatal("delete key=%lld", key);
			--count;
		} else {
			key = k_rand_key();
			k_add(key);
			rc = bt_insert(&tree, key);
			if (rc) fatal("bt_insert key=%lld", key);
			++count;
		}
	}
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs  %g nsecs/op\n", total, (double)total/(double)n);
//	pr_next(&tree);
	if (Option.print) bt_print(&tree);
	pr_audit(&tree);
}
