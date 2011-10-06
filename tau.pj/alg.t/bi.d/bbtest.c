/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

/* Binary B Tree */

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

#include "bbtree.h"
#include "util.h"

static void pr_stats (BbTree_s *tree)
{
	BbStat_s s = bb_stats(tree);
	printf("num nodes=%lld sqrt=%g log2=%g\n"
		"max depth=%lld\n"
		"avg depth=%g\n"
		"num left =%lld\n"
		"num right=%lld\n",
		s.num_nodes, sqrt(s.num_nodes), log(s.num_nodes)/log(2.0),
		s.max_depth,
		(double)s.total_depth / (double)s.num_nodes,
		s.num_left,
		s.num_right);
//  bb_pr_path(tree, s.deepest);
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

void test_bb(int n)
{
	BbTree_s tree = { 0 };
	u64 key;
	int i;

	k_seed(1);
	for (i = 0; i < n; i++) {
		key = k_rand_key();
		bb_insert(&tree, key);
	}
//  bb_audit(&tree);
//  if (Option.print) bb_print(&tree);
	bb_print(&tree);
	pr_stats(&tree);
}

void test_bb_level(int n, int level)
{
	BbTree_s tree = { 0 };
	u64 key;
	s64 count = 0;
	int i;
	u64 start, finish, total;

	k_seed(1);
	k_init();
	start = nsecs();
	for (i = 0; i < n; i++) {
		if (k_should_delete(count, level)) {
			key = k_delete_rand();
			bb_delete(&tree, key);
			--count;
		} else {
			key = k_rand_key();
			k_add(key);
			bb_insert(&tree, key);
			++count;
		}
	}
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs  %g nsecs/op\n", total, (double)total/(double)n);
//printf("\n");
//  bb_audit(&tree);
//  if (Option.print) bb_print(&tree);
	pr_stats(&tree);
}
