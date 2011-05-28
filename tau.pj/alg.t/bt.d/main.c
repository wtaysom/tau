/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

// Needed tests:
// 1. Test for non-existant keys
// 2. Inserting keys that already exist

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mylib.h>
#include <style.h>

#include "bt.h"

enum {	NUM_BUFS = 20 };

struct {
	int	iterations;
	int	level;
	bool	debug;
	bool	print;
} static Option = {
	.iterations = 3000,
	.level = 150,
	.debug = FALSE,
	.print = FALSE };

char *rndstring(unint n)
{
	char	*s;
	unint	j;

	if (!n) return NULL;

	s = malloc(n);
	for (j = 0; j < n-1; j++) {
		s[j] = 'a' + range(26);
	}
	s[j] = 0;
	return s;
}

Lump_s rnd_lump(void)
{
	unint	n;

	n = range(7) + 5;
	return lumpmk(n, rndstring(n));
}

Lump_s fixed_lump(unint n)
{
	return lumpmk(n, rndstring(n));
}

Lump_s seq_lump(void)
{
	enum { MAX_KEY = 4 };

	static int	n = 0;
	int	x = n++;
	char	*s;
	int	i;
	int	r;

	s = malloc(MAX_KEY);
	i = MAX_KEY - 1;
	s[i]   = '\0';
	do {
		--i;
		r = x % 26;
		x /= 26;
		s[i] = 'a' + r;
	} while (x && (i > 0));
	while (--i >= 0) {
		s[i] = ' ';
	}
	return lumpmk(MAX_KEY, s);
}

void pr_lump (Lump_s a) {
	enum { MAX_PRINT = 32 };
	int	i;
	int	size;

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

int pr_rec (Lump_s key, Lump_s val, void *user) {
	u64	*recnum = user;

	printf("%4lld. ", ++*recnum);
	pr_lump(key);
	printf(" : ");
	pr_lump(val);
	printf("\n");
	return 0;
}

void pr_tree (Btree_s *t) {
	u64	recnum = 0;

	printf("******************************\n");
	t_map(t, pr_rec, &recnum);
}

void pr_stats (Btree_s *t) {
	Stat_s	stat = t_get_stats(t);
	u64	records = stat.insert - stat.delete;
	
	printf("Num leaves       = %8lld\n", stat.new_leaves);
	printf("Num branches     = %8lld\n", stat.new_branches);
	printf("Num split leaf   = %8lld\n", stat.split_leaf);
	printf("Num split branch = %8lld\n", stat.split_branch);
	printf("Num insert       = %8lld\n", stat.insert);
	printf("Num find         = %8lld\n", stat.find);
	printf("Num delete       = %8lld\n", stat.delete);
	printf("Num records      = %8lld\n", records);
	printf("Records per leaf = %g\n",
		(double)records / stat.new_leaves);
}

void t_print (Btree_s *t) {
	if (Option.print) t_dump(t);
}

enum {	NUM_BUCKETS = (1 << 20) + 1,
	DYNA_START  = 1,
	DYNA_MAX    = 1 << 27 };

typedef void (*recFunc)(Lump_s key, Lump_s val, void *user);

typedef struct Record_s	Record_s;
struct Record_s {
	Lump_s		key;
	Lump_s		val;
};

static Record_s	*R;
static Record_s	*Next;
static unint	Size;

static void r_add(Lump_s key, Lump_s val)
{
	Record_s *r;

	if (!R) {
		Size = DYNA_START;
		Next = R = emalloc(Size * sizeof(*R));
	}
	if (Next == &R[Size]) {
		unint		newsize;

		if (Size >= DYNA_MAX) {
			fatal("Size %ld > %d", Size, DYNA_MAX);
		}
		newsize = Size << 2;
		R = erealloc(R, newsize * sizeof(*R));
		Next = &R[Size];
		Size = newsize;
	}
	r = Next++;
	r->key = key;
	r->val = val;
}

void r_for_each (recFunc f, void *user)
{
	Record_s *r;

	for (r = R; r < Next; r++) {
		f(r->key, r->val, user);
	}
}

static snint r_rand_index (void)
{
	snint x = Next - R;

	if (!R) return -1;
	if (x == 0) return -1;
	return range(x);
}

Record_s r_get_rand (void)
{
	snint	x = r_rand_index();

	if (x == -1) {
		Record_s r = { Nil, Nil };
		return r;
	}
	return R[x];
}

Lump_s r_delete_rand (void)
{
	snint	x = r_rand_index();
	Lump_s	key;

	if (x == -1) return Nil;
	key = R[x].key;
	R[x] = *--Next;
	return key;
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
	Btree_s *t;
	Lump_s key;
	Lump_s val;
	unint i;

	if (FALSE) seed_random();
	t = t_new(".tfile", NUM_BUFS);
	for (i = 0; i < n; i++) {
		key = seq_lump();
		val = rnd_lump();
		t_insert(t, key, val);
		freelump(key);
		freelump(val);
	}
	t_print(t);
}

void test_rnd(int n)
{
	Btree_s *t;
	Lump_s key;
	Lump_s val;
	unint i;

	if (FALSE) seed_random();
	t = t_new(".tfile", NUM_BUFS);
	for (i = 0; i < n; i++) {
		key = fixed_lump(7);
		val = rnd_lump();
		t_insert(t, key, val);
		freelump(key);
		freelump(val);
	}
//	t_print(t);
//	pr_all_records(t);
//	pr_tree(t);
	t_audit(t);
	pr_stats(t);
}

void find_find (Lump_s key, Lump_s val, void *user) {
	Btree_s	*t = user;
	Lump_s	fval;
	int	rc;

	rc = t_find(t, key, &fval);
	if (rc) {
		fatal("Didn't find %s : rc=%d", key.d, rc);
	} else if (cmplump(val, fval) != 0) {
		fatal("Val not the same %s!=%s", val.d, fval.d);
	} else {
		printf("%s:%s\n", key.d, fval.d);
	}
	freelump(fval);
}

void test_find(int n)
{
	Btree_s *t;
	Lump_s key;
	Lump_s val;
	unint i;

	if (FALSE) seed_random();
	t = t_new(".tfile", NUM_BUFS);
	for (i = 0; i < n; i++) {
		key = fixed_lump(7);
		val = rnd_lump();
		r_add(key, val);
		t_insert(t, key, val);
	}
	r_for_each(find_find, t);
	t_audit(t);
	pr_stats(t);
}

void delete (Lump_s key, Lump_s val, void *user) {
	Btree_s	*t = user;
	int	rc;

	rc = t_delete(t, key);
	if (rc) {
		fatal("Didn't find %s : rc=%d", key.d, rc);
	}
}

void test_delete(int n)
{
	Btree_s *t;
	Lump_s key;
	Lump_s val;
	unint i;

	if (FALSE) seed_random();
	t = t_new(".tfile", NUM_BUFS);
	for (i = 0; i < n; i++) {
		key = fixed_lump(7);
		val = rnd_lump();
		r_add(key, val);
		t_insert(t, key, val);
	}
	r_for_each(delete, t);
	t_audit(t);
	pr_stats(t);
}

int should_delete(s64 count, s64 level)
{
	enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
	return (random() & MASK) * count / level / RANGE;
}

void test_level(int n, int level)
{
	Btree_s	*t;
	Lump_s	key;
	Lump_s	val;
	s64	count = 0;
	unint	i;
	int	rc;

	if (FALSE) seed_random();
	t = t_new(".tfile", NUM_BUFS);
	for (i = 0; i < n; i++) {
t_print(t);
		if (should_delete(count, level)) {
			key = r_delete_rand();
PRlp(key);
			rc = t_delete(t, key);
			if (rc) fatal("delete key=%s", key.d);
			--count;
		} else {
			key = fixed_lump(7);
			val = rnd_lump();
			r_add(key, val);
			rc = t_insert(t, key, val);
			if (rc) fatal("t_insert key=%s val=%s",
					key.d, val.d);
			++count;
		}
	}
	t_audit(t);
	pr_stats(t);
//	t_print(t);
}

void usage(void)
{
	pr_usage("[-dhp] [-i<iterations>] [-l<level>]\n"
		"\t-d - turn on debugging\n"
		"\t-h - print this help message\n"
		"\t-i - num iterations\n",
		"\t-l - level of records to keep\n",
		"\t-p - turn on printing\n");
}

void myoptions(int argc, char *argv[])
{
	int	c;

	fdebugoff();
	setprogname(argv[0]);
	setlocale(LC_NUMERIC, "en_US");
	while ((c = getopt(argc, argv, "dhpi:l:")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			break;
		case 'd':
			Option.debug = TRUE;
			fdebugon();
			break;
		case 'i':
			Option.iterations = strtoll(optarg, NULL, 0);
			break;
		case 'l':
			Option.level = strtoll(optarg, NULL, 0);
			break;
		case 'p':
			Option.print = TRUE;
			break;
		default:
			fatal("unexpected option %c", c);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	myoptions(argc, argv);
//	test_delete(Option.iterations);
	test_level(Option.iterations, Option.level);
	return 0;
}
