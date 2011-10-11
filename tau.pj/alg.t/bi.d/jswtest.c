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

#include "jswtree.h"
#include "util.h"

void test_jsw (int n)
{
	struct jsw_tree tree = { 0 };
	u64 key;
	int i;

	k_seed(1);
	for (i = 0; i < n; i++) {
		key = k_rand_key();
		jsw_insert(&tree, key);
	}
}

void test_jsw_level (int n, int level)
{
	struct jsw_tree tree = { 0 };
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
			rc = jsw_remove(&tree, key);
			if (!rc) fatal("jswremove key=%lld", key);
			--count;
		} else {
			key = k_rand_key();
			k_add(key);
			rc = jsw_insert(&tree, key);
			if (!rc) fatal("jswinsert key=%lld", key);
			++count;
		}
	}
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs  %g nsecs/op\n", total, (double)total/(double)n);
}
