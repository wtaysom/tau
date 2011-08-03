/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Measure time to copy memory. I read some where once upon a time that
 * memcpy is a good first approximation of kernel performance.
 */

#include <sys/resource.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <eprintf.h>
#include <puny.h>
#include <style.h>
#include <timer.h>

bool ResourceUsage = FALSE;
bool UseMemcpy = TRUE;

void PrUsage (struct rusage *r) {
  if (!ResourceUsage) return;
  printf("utime = %ld.%06ld stime = %ld.%06ld minflt = %ld\n",
         r->ru_utime.tv_sec,
         r->ru_utime.tv_usec,
         r->ru_stime.tv_sec,
         r->ru_stime.tv_usec,
         r->ru_minflt);
#if 0      
               struct timeval ru_utime; /* user time used */
               struct timeval ru_stime; /* system time used */
               long   ru_maxrss;        /* maximum resident set size */
               long   ru_ixrss;         /* integral shared memory size */
               long   ru_idrss;         /* integral unshared data size */
               long   ru_isrss;         /* integral unshared stack size */
               long   ru_minflt;        /* page reclaims */
               long   ru_majflt;        /* page faults */
               long   ru_nswap;         /* swaps */
               long   ru_inblock;       /* block input operations */
               long   ru_oublock;       /* block output operations */
               long   ru_msgsnd;        /* messages sent */
               long   ru_msgrcv;        /* messages received */
               long   ru_nsignals;      /* signals received */
               long   ru_nvcsw;         /* voluntary context switches */
               long   ru_nivcsw;        /* involuntary context switches */
#endif
}

void memcpyTest (void) {
  struct rusage before;
  struct rusage after;
  u64 start;
  u64 finish;
  u8 *a;
  u8 *b;
  int n;
  int i;
  int j;
  printf("memcpy\n");
  n = Option.file_size;
  a = emalloc(n);
  b = emalloc(n);
  for (j = 0; j < Option.loops; j++) {
    getrusage(RUSAGE_SELF, &before);
    start = nsecs();
    for (i = Option.iterations; i > 0; i--) {
      memcpy(a, b, n);
    }
    finish = nsecs();
    printf("%d. %g GiB/sec\n", j, 1000000000.0 / (double) (1<<30) *
           (u64)n * (u64)Option.iterations / (double)(finish - start));
    getrusage(RUSAGE_SELF, &after);
    PrUsage(&before);
    PrUsage(&after);
  }
  free(a);
  free(b);
}

void loopTest (void) {
  struct rusage before;
  struct rusage after;
  u64 start;
  u64 finish;
  u64 *a;
  u64 *b;
  u64 *x;
  u64 *y;
  int n;
  int i;
  int j;
  int k;
  printf("loop (%zdbit words)\n", sizeof(*a) * 8);
  n = Option.file_size;
  a = emalloc(n);
  b = emalloc(n);
  for (j = 0; j < Option.loops; j++) {
    getrusage(RUSAGE_SELF, &before);
    start = nsecs();
    for (i = Option.iterations; i > 0; i--) {
      x = a;
      y = b;
      for (k = n / sizeof(*x); k > 0; k--) {
        *x++ = *y++;
      }
    }
    finish = nsecs();
    printf("%d. %g GiB/sec\n", j, 1000000000.0 / (double) (1<<30) *
           (u64)n * Option.iterations / (double)(finish - start));
    getrusage(RUSAGE_SELF, &after);
    PrUsage(&before);
    PrUsage(&after);
    start = nsecs();
  }
  free(a);
  free(b);
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
  if (UseMemcpy) {
    memcpyTest();
  } else {
    loopTest();
  }
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
  case 'm':
    UseMemcpy = FALSE;
    break;
  case 'u':
    ResourceUsage = TRUE;
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void usage (void) {
  pr_usage("-umy -i<iterations> -l<loops> -t<threads> -z<copy size>\n"
           "\th - help\n"
           "\ti - copy buffer i times [%lld]\n"
           "\tl - number of trials to run [%lld]\n"
           "\tm - use own loop [use memcpy]\n"
           "\tt - number of threads [%lld]\n"
           "\tu - resource usage [off]\n"
           "\tz - size of copy buffer in bytes (can use hex) [0x%llx]",
           Option.iterations, Option.loops,
           Option.numthreads, Option.file_size);
}

int main (int argc, char *argv[]) {
  punyopt(argc, argv, myopt, "mu");
  StartThreads();
  return 0;
}
