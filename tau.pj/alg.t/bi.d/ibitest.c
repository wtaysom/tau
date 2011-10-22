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

#include "ibitree.h"
#include "util.h"

static void pr_audit (iBiTree_s *tree)
{
	iBiAudit_s a = ibi_audit(tree);
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
//  ibi_pr_path(tree, s.deepest);
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

static void pr_next (iBiTree_s *tree)
{
	u64 key = 0;
	printf("------\n");
	for (;;) {
		key = ibi_next(tree, key);
		if (key == 0) return;
		printf("%llu\n", key);
	}
}

void test_ibi (int n)
{
	iBiTree_s tree = { 0 };
	u64 key;
	int i;

	k_seed(1);
	for (i = 0; i < n; i++) {
		key = k_rand_key();
		ibi_insert(&tree, key);
	}
	ibi_audit(&tree);
	ibi_print(&tree);
	pr_next(&tree);
	pr_audit(&tree);
}

void test_ibi_level (int n, int level)
{
	iBiTree_s tree = { 0 };
	u64 key;
	s64 count = 0;
	int i;
	int rc;
	u64 start, finish, total;

	k_seed(1);
	k_init();
	start = nsecs();
	for (i = 0; i < n; i++) {
//PRd(i);
//ibi_print(&tree);
		if (k_should_delete(count, level)) {
			key = k_delete_rand();
//PRd(key);
			rc = ibi_delete(&tree, key);
//ibi_print(&tree);
//Pause();
			if (rc) fatal("delete key=%lld", key);
			--count;
		} else {
			key = k_rand_key();
			k_add(key);
			rc = ibi_insert(&tree, key);
			if (rc) fatal("ibi_insert key=%lld", key);
			++count;
		}
	}
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs  %g nsecs/op\n", total, (double)total/(double)n);
	ibi_audit(&tree);
//ibi_print(&tree);
//	pr_next(&tree);
//printf("\n");
//  ibi_audit(&tree);
//  if (Option.print) ibi_print(&tree);
	pr_audit(&tree);
}
