/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

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
	bool	debug;
} static Option = {
	.iterations = 23,
	.debug = FALSE };

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


enum { NUM_BUCKETS = 7 };

typedef void (*recFunc)(Lump_s key, Lump_s val, void *user);

struct Record {
	struct Record *next;
	Lump_s key;
	Lump_s val;
};

struct Record *Bucket[NUM_BUCKETS];

static struct Record *hash(Lump_s key)
{
	unint h = crc32(key.d, key.size) % NUM_BUCKETS;
	return (struct Record *)&Bucket[h];
}

static void h_add(Lump_s key, Lump_s val)
{
	struct Record *h;
	struct Record *r;

	h = hash(key);
	r = malloc(sizeof(*r));
	r->key = key;
	r->val = val;
	r->next = h->next;
	h->next = r;
}

#if 0
static Lump_s h_find(Lump_s key)
{
	struct Record *h;
	struct Record *r;

	h = hash(key);
	r = h->next;
	while (r) {
		if (cmplump(key, r->key) == 0) {
			return r->val;
		}
		r = r->next;
	}
	return Nil;
}
#endif

static void h_for_each (recFunc f, void *user)
{
	unint i;
	struct Record *r;

	for (i = 0; i < NUM_BUCKETS; i++) {
		r = Bucket[i];
		while (r) {
			f(r->key, r->val, user);
		}
	}
}

static void print_f(Lump_s key, Lump_s val, void *t)
{
	Lump_s v;

	v = t_find(t, key);
	printf("%s=%s", key.d, val.d);
	if (cmplump(val, v) != 0) {
		printf("!=%s\n", v.d);
	} else {
		printf("\n");
	}
	freelump(v);
}

void t_print(Btree_s *t)
{
	h_for_each(print_f, t);
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
		h_add(key, val);
		t_insert(t, key, val);
		freelump(key);
		freelump(val);
	}
	t_dump(t);
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
		h_add(key, val);
		t_insert(t, key, val);
		freelump(key);
		freelump(val);
	}
//	t_dump(t);
//	pr_all_records(t);
//	pr_tree(t);
	t_audit(t);
}

void usage(void)
{
	pr_usage("[-dh] [-i<iterations>]\n"
		"\t-d - turn on debugging\n"
		"\t-h - print this help message\n"
		"\t-i - num iterations");
}

void myoptions(int argc, char *argv[])
{
	int	c;

	fdebugoff();
	setprogname(argv[0]);
	setlocale(LC_NUMERIC, "en_US");
	while ((c = getopt(argc, argv, "dhi:")) != -1) {
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
		default:
			fatal("unexpected option %c", c);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	myoptions(argc, argv);
	test_rnd(Option.iterations);
	return 0;
}
