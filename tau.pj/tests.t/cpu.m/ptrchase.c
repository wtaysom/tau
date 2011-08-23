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
#include <mystdlib.h>
#include <puny.h>
#include <q.h>
#include <style.h>
#include <timer.h>

enum { PAGE_SIZE = 1 << 12,
       PAGE_MASK = PAGE_SIZE - 1,
       PRIME = 1610612741 };

u64 NumPages;
u64 Storage;
u64 NumPtrs;

static void **init_pointers (void **p, u64 n, u64 step) {
  void **head;
  void **atom;
  u64 i, j;
  assert(n % PRIME != 0);
  head = NULL;
  memset(p, 0, n * sizeof(*p));
  for (j = 0, i = 0; j < n; j++, i += step) {
    atom = &p[i % n];
    assert(!*atom);
    *atom = head;
    head = atom;
  }
  return head;
}

static void **init_rand_pointers (void **p, u64 n) {
  void **head;
  void **atom;
  u32 *avail;
  u32 k;
  u64 i, j;
  assert(n % PRIME != 0);
  avail = emalloc(n * sizeof(u32));
  for (i = 0; i < n; i++) {
    avail[i] = i;
  }
  head = NULL;
  memset(p, 0, n * sizeof(*p));
  for (j = 0, i = n; j < n; j++) {
    k = urand(i);
    atom = &p[avail[k]];
    avail[k] = avail[--i];
    assert(!*atom);
    *atom = head;
    head = atom;
  }
  return head;
}

static void **init_seq_pointers (void **p, u64 n) {
  void **head;
  void **atom;
  u64 i, j;
  assert(n % PRIME != 0);
  head = NULL;
  memset(p, 0, n * sizeof(*p));
  for (j = 0, i = n - 1; j < n; j++, i--) {
    atom = &p[i];
    assert(!*atom);
    *atom = head;
    head = atom;
  }
  return head;
}

void chase (const char *label, void **head)
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
  printf("%8s %g nsecs/ptr\n", label,
         (double)(finish - start) /
             (double)( Option.iterations * NumPtrs));
}

void chaseTest (void) {
  void **head;
  void **p = eallocpages(NumPages, PAGE_SIZE);
  head = init_seq_pointers(p, NumPtrs);
  chase("seq", head);
  head = init_rand_pointers(p, NumPtrs);
  chase("rand", head);
  head = init_pointers(p, NumPtrs, PRIME);
  chase("prime", head);
  head = init_pointers(p, NumPtrs, 1);
  chase("back", head);
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
  default:
    return FALSE;
  }
  return TRUE;
}

void usage (void) {
  pr_usage("-gh -i<iterations> -l<loops> -t<threads> -z<copy size>\n"
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
  Option.file_size = (1<<12);  /* Using file_size for num pages */
  Option.numthreads = 1;
  punyopt(argc, argv, myopt, "g");
  NumPages = Option.file_size;
  Storage  = NumPages * PAGE_SIZE;
  NumPtrs  = Storage / sizeof(void *);
  StartThreads();
  return 0;
}
