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
#include <mystdlib.h>
#include <style.h>
#include <twister.h>

#include "buf.h"
#include "ht.h"

struct {
	int	numbufs;
	int	iterations;
	int	level;
	bool	debug;
	bool	print;
} static Option = {
	.numbufs = 20,
	.iterations = 3000,
	.level = 150,
	.debug = FALSE,
	.print = FALSE };

char *rndstring(unint n)
{
	char *s;
	unint j;

	if (!n) return NULL;

	s = malloc(n);
	for (j = 0; j < n-1; j++) {
		s[j] = 'a' + twister_urand(26);
	}
	s[j] = 0;
	return s;
}

Lump_s rnd_lump(void)
{
	unint n;

	n = twister_urand(7) + 5;
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

Key_t rnd_key (void)
{
	switch (0) {
	case 0: {
			static Key_t	key = 0;
			return ++key;
		}
	case 1: return twister_random();
	}
}

Key_t seq_key (void)
{
	static Key_t	key = 0;

	return ++key;
}

void pr_cach_stats (Htree_s *t)
{
	CacheStat_s	cs;
	cs = cache_stats();
	printf("cache stats:\n"
		"  num bufs =%8d\n"
		"  gets     =%8lld\n"
		"  puts     =%8lld\n"
		"  hits     =%8lld\n"
		"  miss     =%8lld\n"
		"hit ratio  =%8g%%\n",
		cs.numbufs,
		cs.gets, cs.puts, cs.hits, cs.miss,
		(double)(cs.hits) / (cs.hits + cs.miss) * 100.);
}

void pr_audit (Audit_s *a)
{
	printf("Audit:\n"
		"  branches:  %8lld\n"
		"  leaves:    %8lld\n"
		"  records:   %8lld\n"
		"  recs/leaf: %8g\n",
		a->branches,
		a->leaves, a->records,
		(double)a->records / a->leaves);
}

int audit (Htree_s *t)
{
	Audit_s	a;
	int rc = t_audit(t, &a);
	return rc;
}

void pr_lump (Lump_s a)
{
	enum { MAX_PRINT = 32 };
	int	i;
	int	size;
	char	*d;

	size = a.size;
	d = (char *)a.d;
	if (size > MAX_PRINT) size = MAX_PRINT;
	for (i = 0; i < size; i++) {
		if (isprint(d[i])) {
			putchar(d[i]);
		} else {
			putchar('.');
		}
	}
}

void pr_key (Key_t key)
{
	printf("%llu", (u64)key);
}

static int pr_rec (Hrec_s rec, void *user)
{
	u64 *recnum = user;

	printf("%4lld. ", ++*recnum);
	pr_key(rec.key);
	printf(" = ");
	pr_lump(rec.val);
	printf("\n");
	return 0;
}

void pr_tree (Htree_s *t)
{
	u64 recnum = 0;

	printf("\n**************pr_tree****************\n");
	t_map(t, pr_rec, &recnum);
}

void pr_stats (Htree_s *t)
{
	Stat_s stat = t_get_stats(t);
	u64 records = stat.insert - stat.delete;

	printf("Num new leaves       = %8lld\n", stat.leaf.new);
	printf("Num new branches     = %8lld\n", stat.branch.new);
	printf("Num split leaf       = %8lld\n", stat.leaf.split);
	printf("Num split branch     = %8lld\n", stat.branch.split);
	printf("Num join leaf        = %8lld\n", stat.leaf.join);
	printf("Num join branch      = %8lld\n", stat.branch.join);
	printf("Num deleted leaves   = %8lld\n", stat.leaf.deleted);
	printf("Num deleted branches = %8lld\n", stat.branch.deleted);
	printf("Num insert           = %8lld\n", stat.insert);
	printf("Num delete           = %8lld\n", stat.delete);
	printf("Num find             = %8lld\n", stat.find);
	printf("Num next             = %8lld\n", stat.next);
	printf("Num records          = %8lld\n", records);
	printf("Records per leaf = %g\n",
		(double)records / (stat.leaf.new - stat.leaf.deleted));
}

void t_print (Htree_s *t)
{
	if (Option.print) {
		t_dump(t);
	}
}

enum { NUM_BUCKETS = (1 << 20) + 1,
	DYNA_START  = 1,
	DYNA_MAX    = 1 << 27 };

typedef void (*recFunc)(Key_t key, Lump_s val, void *user);

static Hrec_s *R;
static Hrec_s *Next;
static unint Size;

static void r_add (Key_t key, Lump_s val)
{
	Hrec_s *r;

	if (!R) {
		Size = DYNA_START;
		Next = R = emalloc(Size * sizeof(*R));
	}
	if (Next == &R[Size]) {
		unint  newsize;

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
	Hrec_s *r;

	for (r = R; r < Next; r++) {
		f(r->key, r->val, user);
	}
}

static snint r_rand_index (void)
{
	snint x = Next - R;

	if (!R) return -1;
	if (x == 0) return -1;
	return twister_urand(x);
}

Hrec_s r_get_rand (void)
{
	snint x = r_rand_index();

	if (x == -1) {
		Hrec_s r = { 0, Nil };
		return r;
	}
	return R[x];
}

Key_t r_delete_rand (void)
{
	snint x = r_rand_index();
	Key_t key;

	if (x == -1) return 0;
	key = R[x].key;
	R[x] = *--Next;
	return key;
}

void test1 (void)
{
	int	n = Option.iterations;
	Lump_s	val;
	unint	i;

	for (i = 0; i < n; i++) {
		val = seq_lump();
		printf("%s\n", (char *)val.d);
		freelump(val);
	}
}

void test_seq (Htree_s *t)
{
	int	n = Option.iterations;
	Key_t	key;
	Lump_s	val;
	unint	i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		t_print(t);
		key = seq_key();
		val = rnd_lump();
		t_insert(t, key, val);
		freelump(val);
	}
	t_print(t);
}

void test_rnd (Htree_s *t)
{
	int	n = Option.iterations;
	Key_t	key;
	Lump_s	val;
	unint	i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		key = rnd_key();
		val = rnd_lump();
		t_insert(t, key, val);
		freelump(val);
	}
// t_print(t);
// pr_all_records(t);
// pr_tree(t);
	audit(t);
	pr_stats(t);
}

void find_find (Key_t key, Lump_s val, void *user)
{
	Htree_s	*t = user;
	Lump_s	fval;
	int	rc;

	rc = t_find(t, key, &fval);
	if (rc) {
		fatal("Didn't find %llu : rc=%d", (u64)key, rc);
	} else if (cmplump(val, fval) != 0) {
		fatal("Val not the same %s!=%s", (char *)val.d, (char *)fval.d);
	} else {
		printf("%llu:%s\n", (u64)key, (char *)fval.d);
	}
	freelump(fval);
}

void test_find (Htree_s *t)
{
	int	n = Option.iterations;
	Key_t	key;
	Lump_s	val;
	unint	i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		key = rnd_key();
		val = rnd_lump();
		r_add(key, val);
		t_insert(t, key, val);
	}
	r_for_each(find_find, t);
	audit(t);
	pr_stats(t);
}

void delete (Key_t key, Lump_s val, void *user)
{
	Htree_s *t = user;
	int rc;

	rc = t_delete(t, key);
	if (rc) {
		fatal("Didn't find %llu : rc=%d", (u64)key, rc);
	}
}

void test_delete (Htree_s *t)
{
	int	n = Option.iterations;
	Key_t	key;
	Lump_s	val;
	unint	i;

	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		key = rnd_key();
		val = rnd_lump();
		r_add(key, val);
		t_insert(t, key, val);
	}
	r_for_each(delete, t);
	audit(t);
	pr_stats(t);
}

int should_delete(s64 count, s64 level)
{
	enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
	return (random() & MASK) * count / level / RANGE;
}

void test_level (Htree_s *t)
{
	int	n = Option.iterations;
	int	level = Option.level;
	Key_t	key;
	Lump_s	val;
	s64	count = 0;
	int	i;
	int	rc;

	Option.debug = TRUE;
	if (FALSE) seed_random();
	for (i = 0; i < n; i++) {
		if (should_delete(count, level)) {
			key = r_delete_rand();
			rc = t_delete(t, key);
			if (rc) fatal("delete key=%lld",(u64)key);
			--count;
		} else {
			key = rnd_key();;
			val = rnd_lump();
			r_add(key, val);
			rc = t_insert(t, key, val);
			if (rc) fatal("t_insert key=%lld val=%s",
					(u64)key, val.d);
			++count;
		}
		//audit(t);
	}
	audit(t);
	t_print(t);
	pr_stats(t);
	{
		Audit_s a;
		t_audit(t, &a);
		pr_audit(&a);
	}
	pr_cach_stats(t);
}

void usage(void)
{
	pr_usage("[-dhp] [-i<iterations>] [-l<level>]\n"
		"\t-b - set number of buffers [%d]\n"
		"\t-d - turn on debugging [%s]\n"
		"\t-h - print this help message\n"
		"\t-i - num iterations [%d]\n"
		"\t-l - level of records to keep [%d]\n"
		"\t-p - turn on printing [%s]\n",
		Option.numbufs,
		Option.debug ? "on" : "off",
		Option.iterations,
		Option.level,
		Option.print ? "on" : "off");
}

void myoptions(int argc, char *argv[])
{
	int c;

	fdebugoff();
	setprogname(argv[0]);
	setlocale(LC_NUMERIC, "en_US");
	while ((c = getopt(argc, argv, "dhpb:i:l:")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			break;
		case 'b':
			Option.numbufs = strtoll(optarg, NULL, 0);
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
	myoptions(argc, argv);
	cache_start(Option.numbufs);

#if 0
	Volume_s *v = crfs_create(".tfile");
	Htree_s *t = crfs_htree(v);

	test_level(t);

#else

	void cmd(void);

	cmd();
#endif
	return 0;
}

#if 0
	test_delete(t);
	test_seq(t);
	test_level(t);
#endif
