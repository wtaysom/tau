/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <style.h>
#include <debug.h>
#include <mystdlib.h>
#include <eprintf.h>
#include <cmd.h>

#include "buf.h"
#include "ht.h"
#include "log.h"
#include "twins.h"

/*
 * Work items:
 *	1. tests including setting number of iterations and probabilities
 *		a. delete during enumeration
 *	*2. need to gather statistics - num branches, num records, num leaves,
 *		num branch recs, total free space in leaves and branches
 *	*3. next record
 *		*a. simple recursive
 *		b. my own stack of blocks
 *		c. key to next block
 *			This won't work because the whole next branch might
 *			be deleted?  Nope it can't be.  It has to be merged
 *			with predecessor.
 *		d. try with out locking, if fails, lock whole tree, and try
 *			again.
 *	4. apply for next record
 *	5. Tests:
 *		a. deletes and adds records while enumerating
 *		b. do we want finer grained locking than the
 *			root of the tree?  Traditionally, we
 *			haven't.
 *	6. Persistent btree
 *		*a. when tree grows, must update root block
 *		*b. add dirty blocks
 *		c. changed blocks get assigned a new address
 *	7. Ideas
 *		a. bonjour for connections
 *		b. i need enough memory so I can remap every block
 *		c. 100 servers - 1 Terabytes, even at 64 bits, 4K blocks,
 *			40-12+3=31 or 2 Gigabytes, using 32bit block numbers,
 *			We need 1 Gig, can we remap at a larger granularity?
 *	8. Failures:
 *		*a. gen 20; rm *; leave a few - use the field name for
 *			sizeof, not the type.
 *	9. Hash collisions:
 *		a. string becomes key
 *		b. 56 bit hash : 8 bit unique
 *	10. Make printing functions different from the other debug.
 */

enum {	MAX_VAL = 32,
	MAX_KEY = 1 << 31};

Key_t gen_key (void)
{
	Key_t	key;

	do {
		key = urand(MAX_KEY);
	} while (!key);
	return key;
}

Lump_s gen_val (void)
{
	static u8 val_char[] =	"abcdefghijklmnopqrstuvwxyz"
					//"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";
	static u8	name[MAX_VAL + 2];
	Lump_s		val;
	u8		*c;
	unsigned	len;

	len = (urand(MAX_VAL) + urand(MAX_VAL) + urand(MAX_VAL)) / 4 + 1;
	for (c = name; len; c++, len--) {
		*c = val_char[urand(sizeof(val_char) - 1)];
	}
	*c = '\0';
	val.d = name;
	val.size = len + 1;
	return val;
}

Lump_s mk_val (Key_t key)
{
	static char	name[MAX_VAL + 1];
	Lump_s	val;

	snprintf(name, MAX_VAL, "%lld", (u64)key);
	return lumpmk(strlen(name) + 1, name);
	return val;
}

int apply (int (*fn)(Key_t, void *), void *data)
{
	Key_t	key;
	int	rc;
	Lump_s	val;

	for (key = 0;;) {
		if (next_twins(key, &key, &val) != 0) {
			break;
		}
		rc = fn(key, data);
		if (rc) {
			return rc;
		}
	}
	return 0;
}

int expand (Key_t target_key, int (fn)(Key_t))
{
	int	rc;
	Key_t	key;
	Lump_s	val;
FN;
	for (key = 0;;) {
		rc = next_twins(key, &key, &val);
		if (rc) break;
		if (key == target_key) {
			rc = fn(key);
			if (rc) {
				return rc;
			}
		}
	}
	return 0;
}

int dp (int argc, char *argv[])
{
	Key_t	key;
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		key = strtoll(argv[i], NULL, 0);
		rc = delete_twins(key);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

int ep (int argc, char *argvp[])
{
	Key_t	key;
	Lump_s	val;
	int	rc;

	for (key = 0; key != MAX_KEY;) {
		rc = next_twins(key, &key, &val);
		if (rc) break;
		printf("%llu %s\n", (u64)key, (char *)val.d);
	}
	return 0;
}

int ip (int argc, char *argv[])
{
	Key_t	key;
	Lump_s	val;
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		key = strtoll(argv[i], NULL, 0);
		val = lumpmk(strlen(argv[i]) + 1, argv[i]);
		rc = insert_twins(key, val);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

int fp (int argc, char *argv[])
{
	Key_t	key;
	Lump_s	val;
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		key = strtoll(argv[i], NULL, 0);
		rc = find_twins(key, &val);
		if (rc != 0) {
			printf("Didn't find string %s\n", argv[i]);
			return rc;
		}
		if (strcmp(argv[i], val.d) != 0) {
			printf("Val wrong for %llx %s %s\n",
				(u64)key, argv[i], (char *)val.d);
		}
		freelump(val);
	}
	return 0;
}

int pp (int argc, char *argv[])
{
	t_dump(TreeA);
	t_dump(TreeB);
	return 0;
}

int qp (int argc, char *argv[])
{
	exit(0);
	return 0;
}

int rmv (Key_t key)
{
	return delete_twins(key);
}

int rmp (int argc, char *argv[])
{
	Key_t	key;
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		key = strtoll(argv[i], NULL, 0);
		rc = expand(key, rmv);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

#if 0
Need to copy some of these statistics. In htree they are split
between audit and stat. Audit requires a tree traversal, stat
does not.
void st_tree (Htree_s *tree)
{
	btree_stat_s	bs;

	stat_tree(tree, &bs);
	printf("block size      = %lld\n", bs.st_blk_size);
	printf("num leaves      = %lld\n", bs.st_num_leaves);
	printf("num branches    = %lld\n", bs.st_num_branches);
	printf("leaf free space = %lld\n", bs.st_leaf_free_space);
	printf("leaf recs       = %lld\n", bs.st_leaf_recs);
	printf("branch recs     = %lld\n", bs.st_branch_recs);
	printf("height          = %lld\n", bs.st_height);
	printf("Percent full    = %.3g%%\n",
		(((double)bs.st_num_leaves * bs.st_blk_size)
			- (double)bs.st_leaf_free_space)
		/ ((double)bs.st_num_leaves * bs.st_blk_size) * 100.);
	printf("Recs per branch = %.3g\n",
		(double)bs.st_branch_recs / (double)bs.st_num_branches);
	printf("Recs per leaf   = %.3g\n",
		(double)bs.st_leaf_recs / (double)bs.st_num_leaves);
}
#endif

int stp (int argc, char *argv[])
{
	printf("A:\n");
	pr_stats(TreeA);
	printf("B:\n");
	pr_stats(TreeB);
	return 0;
}

int cp (int argc, char *argv[])
{
	Stat_s	a;
	Stat_s	b;

	a = t_get_stats(TreeA);
	b = t_get_stats(TreeB);
	if (memcmp( &a, &b, sizeof(a)) != 0) {
		printf("We have a problem Huston.\n");
		stp(argc, argv);
	}
	return t_compare_trees(TreeA, TreeB);
}

int clearp (int argc, char *argv[])
{
	cache_invalidate();
	return 0;
}

int rp (int argc, char *argv[])
{
	cache_invalidate();
	replay_log(VolA);
	return 0;
}

int genp (int argc, char *argv[])
{
	int	n;
	int	i;
	int	rc;
	Key_t	key;

	if (argc > 1) {
		n = atoi(argv[1]);
	} else {
		n = 10;
	}
	for (i = 0; i < n; i++) {
		do {
			key = gen_key();
		} while (inuse_twins(key));
		rc = insert_twins(key, mk_val(key));
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

static int num_recs_cnt (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	int	*cnt = data;

	++*cnt;
	return HT_TRY_NEXT;
}

int num_recs (void)
{
	int		sum_a = 0;
	int		sum_b = 0;

	t_search(TreeA, 0, num_recs_cnt, &sum_a);
	t_search(TreeB, 0, num_recs_cnt, &sum_b);
	if (sum_a != sum_b) {
		printf("Sums don't match %d!=%d\n", sum_a, sum_b);
	}
	return sum_a;
}

typedef struct ith_search_s {
	int	sum;
	int	ith;
	u64	*key;
} ith_search_s;

static int ith_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	ith_search_s	*s = data;

	if (s->sum == s->ith) {
		*s->key = rec_key;
		return 0;
	}
	++s->sum;
	return HT_TRY_NEXT;
}

int find_ith (Htree_s *tree, int ith, u64 *key)
{
	ith_search_s	s;

	s.sum = 0;
	s.ith = ith;
	s.key = key;
	return t_search(tree, 0, ith_search, &s);
}

int del_ith (int ith)
{
	int	rc_a;
	int	rc_b;
	u64	key_a;
	u64	key_b;

	rc_a = find_ith(TreeA, ith, &key_a);
	if (rc_a) {
		printf("TreeA couldn't find record %d, err = %d\n", ith, rc_a);
		return rc_a;
	}
	rc_b = find_ith(TreeB, ith, &key_b);
	if (rc_b) {
		printf("TreeA couldn't find record %d, err = %d\n", ith, rc_b);
		return rc_b;
	}
	if (key_a != key_b) {
		printf("Not the same %llx!=%llx\n", key_a, key_b);
	}
	rc_a = t_delete(TreeA, key_a);
	rc_b = t_delete(TreeB, key_b);
	if (rc_a || rc_b) {
		printf("Problem deleting keys %d:%d %llx:%llx\n",
			rc_a, rc_b, key_a, key_b);
	}
	return rc_a;
}

int mixp (int argc, char *argv[])
{
	int	n;
	int	i;
	int	rc;
	int	sum;
	Key_t	key;
	int	x;

	if (argc > 1) {
		n = atoi(argv[1]);
	} else {
		n = 10;
	}
	sum = num_recs();

	for (i = 0; i < n; i++) {
		if (!sum || random_percent(51)) {
			do {
				key = gen_key();
			} while (inuse_twins(key));
			rc = insert_twins(key, mk_val(key));
			if (rc != 0) {
				return rc;
			}
			++sum;
			x = num_recs();
			assert(sum == x);
		} else {
			x = urand(sum);
			rc = del_ith(x);//urand(sum));
			if (rc) {
				return rc;
			}
			--sum;
			x = num_recs();
			assert(sum == x);
		}
	}
	return 0;
}

int fill (int n)
{
	int	i;
	int	rc;
	Key_t	key;
	Lump_s	val;

	for (i = 0; i < n; i++) {
		do {
			key = gen_key();
		} while (find_twins(key, &val) != HT_ERR_NOT_FOUND);
		rc = insert_twins(key, mk_val(key));
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

int rmv_percent (Key_t key, void *p)
{
	int	x = *(int *)p;

	if (random_percent(x)) {
		return delete_twins(key);
	}
	return 0;
}

int empty (int x)
{
	return apply(rmv_percent, &x);
}

int t1p (int argc, char *argv[])
{
	int	n;
	int	p;
	int	i;
	int	rc = 0;

	n = 10;
	p = 75;
	if (argc > 1) {
		p = atoi(argv[1]);
	} if (argc > 2) {
		n = atoi(argv[2]);
	}
	rc = fill(100);
	for (i = 0; !rc && (i < n); i++) {
		if (random_percent(75)) {
			rc = fill(100);
		} else {
			rc = empty(p);
		}
	}
	return rc;
}

int inusep (int argc, char *argv[])
{
	//baudit("cmd line");
	cache_balanced();
	return 0;
}

void init_cmd (void)
{
	CMD(inuse, "# dump inuse cache buffers");
	CMD(t1,	"[percent [n]] # test 1");
	CMD(mix, "[n]  # do n random adds and deletes, defaults to 10");
	CMD(gen, "[n]  # generate n random keys, defaults to 10");
	CMD(st,	"      # collect statistics on btree");
	CMD(rm,	"[re]+ # remove keys matching regular expressions");
	CMD(q,	"       # quit");
	CMD(p,	"       # print tree");
	CMD(f,	"[key]+ # find given keys");
	CMD(e,	"       # enumerate");
	CMD(i,	"[key]+ # insert given keys");
	CMD(d,	"[key]+ # delete given keys");
	CMD(c,	"       # compare the two trees");
	CMD(r,	"       # replay log");
	CMD(clear,	"       # clear buffer cache");
}

void cmd (void)
{
	init_twins(".devA", ".devB");
	init_shell(NULL);
	init_cmd();
	if (!shell()) warn("shell error");
}
