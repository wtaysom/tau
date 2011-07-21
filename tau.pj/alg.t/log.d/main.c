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

#include <tauerr.h>
#include <btree.h>
#include <twins.h>
#include <blk.h>

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

char *gen_name (void)
{
	static char file_name_char[] =	"abcdefghijklmnopqrstuvwxyz"
					//"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";
	static char	name[MAX_NAME + 2];
	char		*c;
	unsigned	len;

	len = (urand(MAX_NAME) + urand(MAX_NAME) + urand(MAX_NAME)) / 5 + 1;
	for (c = name; len; c++, len--) {
		*c = file_name_char[urand(sizeof(file_name_char) - 1)];
	}
	*c = '\0';
	return name;
}

int apply (int (*fn)(char *, void *), void *data)
{
	u64	key;
	int	rc;
	char	name[MAX_NAME+1];

	for (key = 0;;) {
		if (next_twins(key, &key, name) != 0) {
			break;
		}
		rc = fn(name, data);
		if (rc) {
			return rc;
		}
	}
	return 0;
}

int expand (char *p, int (fn)(char *))
{
	u64	key;
	int	rc;
	char	name[MAX_NAME+1];
FN;
	for (key = 0;;) {
		rc = next_twins(key, &key, name);
PRs(name);
		if (rc) break;
		if (isMatch(p, name)) {
			rc = fn(name);
			if (rc) {
				return rc;
			}
		}
	}
	return 0;
}

int dp (int argc, char *argv[])
{
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		rc = delete_twins(argv[i]);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

int ep (int argc, char *argvp[])
{
	u64	key;
	char	name[MAX_NAME+1];
	int	rc;

	for (key = 0; key != 0xffffffffffffffffULL;) {
		rc = next_twins(key, &key, name);
		if (rc) break;
		printf("%llu %s\n", key-1, name);
	}
	return 0;
}

int ip (int argc, char *argv[])
{
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		rc = insert_twins(argv[i]);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

int fp (int argc, char *argv[])
{
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		rc = find_twins(argv[i]);
		if (rc != 0) {
			printf("Didn't find string %s\n", argv[i]);
			return rc;
		}
	}
	return 0;
}

int pp (int argc, char *argv[])
{
	dump_tree( &TreeA);
	dump_tree( &TreeB);
	return 0;
}

int qp (int argc, char *argv[])
{
	exit(0);
	return 0;
}

int rmv (char *name)
{
	char	key[1024];
	// This is kind of dangerous.  Should make a separate copy
	// of the name.  Actually, shouldn't pass back pointers.
	strcpy(key, name);
	return delete_twins(key);
}

int rmp (int argc, char *argv[])
{
	int	i;
	int	rc;

	for (i = 1; i < argc; ++i) {
		rc = expand(argv[i], rmv);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

void st_tree (tree_s *tree)
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

int stp (int argc, char *argv[])
{
	printf("A:\n");
	st_tree( &TreeA);
	printf("B:\n");
	st_tree( &TreeB);
	return 0;
}

int cp (int argc, char *argv[])
{
	btree_stat_s	bs_a;
	btree_stat_s	bs_b;

	stat_tree( &TreeA, &bs_a);
	stat_tree( &TreeB, &bs_b);
	if (memcmp( &bs_a, &bs_b, sizeof(bs_a)) != 0) {
		printf("We have a problem Huston.\n");
		stp(argc, argv);
	}
	return compare_trees( &TreeA, &TreeB);
}

int clearp (int argc, char *argv[])
{
	bclear();
	return 0;
}

int rp (int argc, char *argv[])
{
	bclear();
	return replay_log(TreeA.t_log);
}

int genp (int argc, char *argv[])
{
	int	n;
	int	i;
	int	rc;
	char	*name;

	if (argc > 1) {
		n = atoi(argv[1]);
	} else {
		n = 10;
	}
	for (i = 0; i < n; i++) {
		do {
			name = gen_name();
		} while (find_twins(name) == 0);
		rc = insert_twins(name);
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
	return qERR_TRY_NEXT;
}

int num_recs (void)
{
	int		sum_a = 0;
	int		sum_b = 0;

	search( &TreeA, 0, num_recs_cnt, &sum_a);
	search( &TreeB, 0, num_recs_cnt, &sum_b);
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
	return qERR_TRY_NEXT;
}

int find_ith (tree_s *tree, int ith, u64 *key)
{
	ith_search_s	s;

	s.sum = 0;
	s.ith = ith;
	s.key = key;
	return search( tree, 0, ith_search, &s);
}

int del_ith (int ith)
{
	int	rc_a;
	int	rc_b;
	u64	key_a;
	u64	key_b;

	rc_a = find_ith( &TreeA, ith, &key_a);
	if (rc_a) {
		printf("TreeA couldn't find record %d, err = %d\n", ith, rc_a);
		return rc_a;
	}
	rc_b = find_ith( &TreeB, ith, &key_b);
	if (rc_b) {
		printf("TreeA couldn't find record %d, err = %d\n", ith, rc_b);
		return rc_b;
	}
	if (key_a != key_b) {
		printf("Not the same %llx!=%llx\n", key_a, key_b);
	}
	rc_a = delete( &TreeA, key_a);
	rc_b = delete( &TreeB, key_b);
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
	char	*name;
	int	x;

	if (argc > 1) {
		n = atoi(argv[1]);
	} else {
		n = 10;
	}
	sum = num_recs();

	for (i = 0; i < n; i++) {
		if (!sum || percent(51)) {
			do {
				name = gen_name();
			} while (find_twins(name) != qERR_NOT_FOUND);
			rc = insert_twins(name);
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
	char	*name;

	for (i = 0; i < n; i++) {
		do {
			name = gen_name();
		} while (find_twins(name) != qERR_NOT_FOUND);
		rc = insert_twins(name);
		if (rc != 0) {
			return rc;
		}
	}
	return 0;
}

int rmv_percent (char *name, void *p)
{
	int	x = *(int *)p;

	if (percent(x)) {
		return delete_twins(name);
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
		if (percent(75)) {
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
	binuse();
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

int main (int argc, char *argv[])
{
	char	*devA = ".btreeA";
	char	*devB = ".btreeB";
	char	*logA = ".logA";
	char	*logB = ".logB";

#if 1
	fdebugon();
#else
	fdebugoff();
#endif

#if 1
	debugon();
#else
	debugoff();
#endif
FN;
	if (argc > 1) {
		devA = argv[1];
	}
	if (argc > 2) {
		devB = argv[1];
	}
	if (argc > 3) {
		logA = argv[1];
	}
	if (argc > 4) {
		logB = argv[1];
	}
#if 1
	unlink(devA);	/* Development only */
	unlink(devB);	/* Development only */
	unlink(logA);	/* Development only */
	unlink(logB);	/* Development only */
#endif
	binit(20);
	init_twins(devA, logA, devB, logB);
	init_shell(NULL);
	init_cmd();
	return shell();
}
