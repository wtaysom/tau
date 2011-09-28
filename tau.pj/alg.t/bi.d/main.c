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

enum { NUM_BUFS = 10 };

Option_s Option = {
  .iterations = 3000,
  .level = 150,
  .debug = FALSE,
  .print = FALSE };

char *rndstring(unint n) {
  char *s;
  unint j;

  if (!n) return NULL;

  s = malloc(n);
  for (j = 0; j < n-1; j++) {
    s[j] = 'a' + urand(26);
  }
  s[j] = 0;
  return s;
}

Lump_s rnd_lump(void) {
  unint n;

  n = urand(7) + 5;
  return lumpmk(n, rndstring(n));
}

Lump_s fixed_lump(unint n) {
  return lumpmk(n, rndstring(n));
}

Lump_s seq_lump(void) {
  enum { MAX_KEY = 4 };

  static int n = 0;
  int x = n++;
  char *s;
  int i;
  int r;

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

static void pr_lump (Lump_s a) {
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

/*static*/ int pr_rec (Rec_s rec, BiNode_s *root, void *user) {
  u64 *recnum = user;

  printf("%4lld. ", ++*recnum);
  pr_lump(rec.key);
  printf(" = ");
  pr_lump(rec.val);
  printf("\n");
  return 0;
}

static void pr_stats (BiTree_s *tree) {
  BiStat_s s = bi_stats(tree);
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
  bi_pr_path(tree, s.deepest);
}

enum { NUM_BUCKETS = (1 << 20) + 1,
  DYNA_START  = 1,
  DYNA_MAX    = 1 << 27 };

typedef void (*recFunc)(Lump_s key, Lump_s val, void *user);

static Rec_s *R;
static Rec_s *Next;
static unint Size;

static void r_add (Rec_s rec) {
  Rec_s *r;

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
  *r = rec;
}

static void r_for_each (recFunc f, void *user) {
  Rec_s *r;

  for (r = R; r < Next; r++) {
    f(r->key, r->val, user);
  }
}

static snint r_rand_index (void) {
  snint x = Next - R;

  if (!R) return -1;
  if (x == 0) return -1;
  return urand(x);
}

/*static*/ Rec_s r_get_rand (void) {
  snint x = r_rand_index();

  if (x == -1) {
    Rec_s r = { Nil, Nil };
    return r;
  }
  return R[x];
}

static Lump_s r_delete_rand (void) {
  snint x = r_rand_index();
  Lump_s key;

  if (x == -1) return Nil;
  key = R[x].key;
  R[x] = *--Next;
  return key;
}

void test1(int n) {
  Lump_s key;
  unint i;

  for (i = 0; i < n; i++) {
    key = seq_lump();
    printf("%s\n", key.d);
    freelump(key);
  }
}

void test_seq(int n) {
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

void test_rnd(int n) {
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

void find_find (Lump_s key, Lump_s val, void *user) {
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

void test_bi_find(int n) {
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

void delete (Lump_s key, Lump_s val, void *user) {
  BiTree_s *tree = user;
  int rc;

  rc = bi_delete(tree, key);
  if (rc) {
    fatal("Didn't find %s : rc=%d", key.d, rc);
  }
}

void test_bi_delete(int n) {
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
  pr_stats(&tree);
}

int should_delete(s64 count, s64 level) {
  enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
  return (random() & MASK) * count / level / RANGE;
}

#if 0
if (i >= 595) {
  fdebugon();
  Option.debug = TRUE;
  Option.print = TRUE;
printf("=========%d======\n", i);
}
if (i > 596) {
  fdebugoff();
  Option.debug = FALSE;
  Option.print = FALSE;
}
if (Option.print) {
  printf("delete\n");
  PRlp(key);
  bi_print(root);
}
#endif

void test_level(int n, int level) {
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
  pr_stats(&tree);
}

void usage(void) {
  pr_usage("[-dhp] [-i<iterations>] [-l<level>]\n"
    "\t-d - turn on debugging [%s]\n"
    "\t-h - print this help message\n"
    "\t-i - num iterations [%d]\n"
    "\t-l - level of records to keep [%d]\n"
    "\t-p - turn on printing [%s]\n",
    Option.debug ? "on" : "off",
    Option.iterations,
    Option.level,
    Option.print ? "on" : "off");
}

void myoptions(int argc, char *argv[]) {
  int c;

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

int main(int argc, char *argv[]) {
  myoptions(argc, argv);
  test_rb(Option.iterations, Option.level);
  return 0;
}

#if 0
  test_perf(Option.iterations, Option.level);
  test_bi_delete(Option.iterations);
  test_level(Option.iterations, Option.level);
#endif
