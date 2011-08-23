/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Measure the cost of chasing pointers
 * 1. Sequential set of pointers
 * 2. Use a large prime number for a step size
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <puny.h>
#include <q.h>
#include <style.h>
#include <timer.h>

enum { PAGE_SIZE = 1 << 12,
       PAGE_MASK = PAGE_SIZE - 1,
       PRIME = 1610612741 };

struct {
  bool power10;
  double scale;
  char *units;
  char *legend;
} Gig = { FALSE, 1000000000.0 / ((double)(1<<30)), "GiB", "Gig = 2**30" };

u64 NumPages;
u64 Storage;
u64 NumPtrs;

static void **init_pointers (void **p, u64 n) {
  void **head;
  void **atom;
  u64 i, j;
  assert(n % PRIME != 0);
  head = NULL;
  memset(p, 0, n * sizeof(*p));
  for (j = 0, i = 0; j < n; j++, i += PRIME) {
    atom = &p[i % n];
    assert(!*atom);
    *atom = head;
    head = atom;
  }
  return head;
}

void chase (void **head)
{
  int i;
  u64 start;
  u64 finish;
  void **next;
  start = nsecs();
  for (i = Option.iterations; i > 0; i--) {
    next = *head;
    while (next) {
      next = *next;
    }
  }
  finish = nsecs();
  printf("%g nsecs/ptr\n", (double)(finish - start) /
                           (double)( Option.iterations * NumPtrs));
}

void chaseTest (void) {
  void **head;
  void **p = eallocpages(NumPages, PAGE_SIZE);
  head = init_pointers(p, NumPages * PAGE_SIZE / sizeof(void *));
  chase(head);
}

pthread_mutex_t	StartLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t	WaitLock = PTHREAD_MUTEX_INITIALIZER;
int Wait;

static void Ready (void) {
  pthread_mutex_lock(&WaitLock);
  --Wait;
  pthread_mutex_unlock(&WaitLock);
  pthread_mutex_lock(&StartLock);
  pthread_mutex_unlock(&StartLock);
}

void *RunTest (void *arg) {
  Ready();
  chaseTest();
  return NULL;
}

void StartThreads (void)
{
  pthread_t *thread;
  unsigned i;
  int rc;

  Wait = Option.numthreads;
  pthread_mutex_lock(&StartLock);
  thread = ezalloc(Option.numthreads * sizeof(pthread_t));
  for (i = 0; i < Option.numthreads; i++) {
    rc = pthread_create( &thread[i], NULL, RunTest, NULL);
    if (rc) {
      eprintf("pthread_create %d\n", rc);
      break;
    }
  }
  for (i = 0; Wait; i++) {
    sleep(1);
  }
  pthread_mutex_unlock(&StartLock);
  for (i = 0; i <  Option.numthreads; i++) {
    pthread_join(thread[i], NULL);
  }
}

bool myopt (int c) {
  switch (c) {
  case 'g':
    Gig.power10 = TRUE;
    Gig.scale   = 1.0;
    Gig.units   = "GB";
    Gig.legend  = "Gig = 10**9";
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void usage (void) {
  pr_usage("-gh -i<iterations> -l<loops> -t<threads> -z<copy size>\n"
           "\tg - use Gig == 10**9 [2**30]\n"
           "\th - help\n"
           "\ti - copy buffer i times [%lld]\n"
           "\tl - number of trials to run [%lld]\n"
           "\tt - number of threads [%lld]\n"
           "\tz - numbers of pages (can use hex) [0x%llx]",
           Option.iterations, Option.loops,
           Option.numthreads, Option.file_size);
}

int main (int argc, char *argv[]) {
  Option.iterations = 10;
  Option.loops = 4;
  Option.file_size = (1<<12);
  Option.numthreads = 1;
  punyopt(argc, argv, myopt, "g");
  NumPages = Option.file_size;
  Storage  = NumPages * PAGE_SIZE;
  NumPtrs  = Storage / sizeof(void *);
  StartThreads();
  return 0;
}
