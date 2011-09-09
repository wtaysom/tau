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
#include <style.h>
#include <timer.h>

enum { PAGE_SIZE = 1 << 12,
       PAGE_MASK = PAGE_SIZE - 1,
       PRIME = 1610612741 };

u64 NumPages;
u64 MaxPointers;
u64 StepSize = 10;

static void **init_pointers (void **p, u64 n, u64 step) {
  void **head;
  void **atom;
  u64 i, j;
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

static void **init_rand_pointers (void **p, u64 num_pointers) {
  void **head;
  void **atom;
  u32 *avail;
  u32 k;
  u64 i, j;
  avail = emalloc(num_pointers * sizeof(u32));
  for (i = 0; i < num_pointers; i++) {
    avail[i] = i;
  }
  head = NULL;
  memset(p, 0, num_pointers * sizeof(*p));
  for (j = 0, i = num_pointers; j < num_pointers; j++) {
    k = urand(i);
    atom = &p[avail[k]];
    avail[k] = avail[--i];
    assert(!*atom);
    *atom = head;
    head = atom;
  }
  return head;
}

static void **init_seq_pointers (void **p, u64 num_pointers) {
  void **head;
  void **atom;
  u64 i, j;
  head = NULL;
  memset(p, 0, num_pointers * sizeof(*p));
  for (j = 0, i = num_pointers - 1; j < num_pointers; j++, i--) {
    atom = &p[i];
    assert(!*atom);
    *atom = head;
    head = atom;
  }
  return head;
}

void chase (const char *label, void **head, u64 num_pointers, u64 iter)
{
  int i;
  u64 start;
  u64 finish;
  void **next;
  start = nsecs();
  for (i = iter; i > 0; i--) {
    next = *head;
    while (next) {
      next = *next;
    }
  }
  finish = nsecs();
  printf("%12lld %8s %#6.4g nsecs/ptr\n", num_pointers, label,
         (double)(finish - start) /
             (double)(iter * num_pointers));
}

void chaseTest (void) {
  u64 num_pointers = 1;
  u64 iter = MaxPointers * Option.iterations;
  void **head;
  void **p = eallocpages(NumPages, PAGE_SIZE);
  for (num_pointers = 1;;) {
    printf("\n");
    head = init_seq_pointers(p, num_pointers);
    chase("seq", head, num_pointers, iter);
    head = init_pointers(p, num_pointers, 1);
    chase("back", head, num_pointers, iter);
    head = init_rand_pointers(p, num_pointers);
    chase("rand", head, num_pointers, iter);
    if (num_pointers % PRIME == 0) {
      printf("Skipping prime interval test because num_pointers = %lld\n",
             num_pointers);
    } else {
      head = init_pointers(p, num_pointers, PRIME);
      chase("prime", head, num_pointers, iter);
    }
    if (num_pointers == MaxPointers) break;
    num_pointers *= StepSize;
    if (num_pointers > MaxPointers) num_pointers = MaxPointers;
    iter /= StepSize;
    if (iter == 0) iter = 1;
  }
}

pthread_mutex_t StartLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t WaitLock = PTHREAD_MUTEX_INITIALIZER;
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

static bool myopt (int c) {
  switch (c) {
  case 'g':
    usage();
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void usage (void) {
  pr_usage("-hg -i<iterations> -t<threads> -z<copy size>\n"
           "\th - help\n"
           "\tg - use huge pages (think gigantic - not yet implemented)\n"
           "\ti - copy buffer i times [%lld]\n"
           "\tt - number of threads [%lld]\n"
           "\tz - max pointers to use (can use hex) [%lld]",
           Option.iterations, Option.numthreads, Option.file_size);
}

int main (int argc, char *argv[]) {
  Option.iterations = 10;
  Option.loops = 4;
  Option.file_size = 10000000;  /* Number of pointers to use */
  Option.numthreads = 1;
  punyopt(argc, argv, myopt, "g");
  MaxPointers = Option.file_size;
  NumPages = (MaxPointers * sizeof(void *) + PAGE_SIZE - 1) / PAGE_SIZE;
  StartThreads();
  return 0;
}
