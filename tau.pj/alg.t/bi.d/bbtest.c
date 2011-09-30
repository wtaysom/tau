/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

/* Binary B trees */

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

#include "main.h"
#include "bbtree.h"

static void pr_stats (BbTree_s *tree) {
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

enum { NUM_BUCKETS = (1 << 20) + 1,
       DYNA_START  = 1,
       DYNA_MAX    = 1 << 27 };

typedef void (*recFunc)(u64 key, void *user);

static u64 *K;
static u64 *Next;
static unint Size;

static void k_add (u64 key) {
  if (!K) {
    Size = DYNA_START;
    Next = K = emalloc(Size * sizeof(*K));
  }
  if (Next == &K[Size]) {
    unint  newsize;

    if (Size >= DYNA_MAX) {
      fatal("Size %ld > %d", Size, DYNA_MAX);
    }
    newsize = Size << 2;
    K = erealloc(K, newsize * sizeof(*K));
    Next = &K[Size];
    Size = newsize;
  }
  *Next++ = key;
}

#if 0
static void k_for_each (recFunc f, void *user) {
  u64 *k;

  for (k = K; k < Next; k++) {
    f(*k, user);
  }
}
#endif

static snint k_rand_index (void) {
  snint x = Next - K;

  if (!K) return -1;
  if (x == 0) return -1;
  return urand(x);
}

#if 0
static u64 k_get_rand (void) {
  snint x = k_rand_index();

  if (x == -1) {
    return 0;
  }
  return K[x];
}
#endif

static u64 k_delete_rand (void) {
  snint x = k_rand_index();
  u64 key;

  if (x == -1) return 0;
  key = K[x];
  K[x] = *--Next;
  return key;
}

static int should_delete(s64 count, s64 level) {
#if 0
  enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
  return (random() & MASK) * count / level / RANGE;
#else
  return 0;
#endif
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

void test_bb (int n, int level) {
  BbTree_s tree = { 0 };
  u64 key;
  s64 count = 0;
  int i;
  int rc;
  u64 start, finish, total;

  srandom(1);
  start = nsecs();
  for (i = 0; i < n; i++) {
bb_print(&tree);
    if (should_delete(count, level)) {
      key = k_delete_rand();
      rc = bb_delete(&tree, key);
      if (rc) fatal("delete key=%lld", key);
      --count;
    } else {
      //key = (u64)random() * (u64)random();
      key = i;
      k_add(key);
      rc = bb_insert(&tree, key);
      if (rc) fatal("bb_insert key=%lld", key);
      ++count;
    }
  }
  finish = nsecs();
  total = finish - start;
  printf("%lld nsecs  %g nsecs/op\n", total, (double)total/(double)n);
//printf("\n");
  bb_audit(&tree);
  if (Option.print) bb_print(&tree);
  pr_stats(&tree);
}
