/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Measure time to copy memory. Once upon a time, I read some where that
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
#include <twister.h>

typedef void *(*memcpy_f)(void *dst, const void *src, size_t count);
typedef void *(*memset_f)(void *dst, int c, size_t count);
typedef int (*summem_f)(const void *src, size_t count);

struct {
	double scale;
	char *units;
	char *legend;
} meg = { 1e9 / ((double)(1<<20)), "MiB", "Meg = 2**20" };

u8 resource_usage = FALSE;
u8 bidirectional = TRUE;
u8 init_buffers = TRUE;

void PrUsage (struct rusage *r)
{
	if (!resource_usage) return;
	printf("utime = %ld.%06ld stime = %ld.%06ld minflt = %ld\n",
		 r->ru_utime.tv_sec,
		 (long)r->ru_utime.tv_usec,
		 r->ru_stime.tv_sec,
		 (long)r->ru_stime.tv_usec,
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

enum { ALIGNMENT = 4096 };

void *alloc_aligned (size_t nbytes)
{
	void	*p;
	int	rc;

	rc = posix_memalign(&p, ALIGNMENT,
		((nbytes + ALIGNMENT - 1 ) / ALIGNMENT) * ALIGNMENT);
	if (rc) {
		fatal("posix_memalign %d:", rc);
	}
	return p;
}

void *memcpyGlibc (void *dst, const void *src, size_t n)
{
	return memcpy(dst, src, n);
}

void *memcpySimple (void *dst, const void *src, size_t n)
{
	u8 *d = dst;
	const u8 *s = src;
	while (n-- != 0) *d++ = *s++;
	return dst;
}

void *memcpy32 (void *dst, const void *src, size_t n)
{
	u32 *d = dst;
	const u32 *s = src;
	n /= sizeof(*d);
	while (n-- != 0) *d++ = *s++;
	return dst;
}

void *memcpy64 (void *dst, const void *src, size_t n)
{
	u64 *d = dst;
	const u64 *s = src;
	n /= sizeof(*d);
	while (n-- != 0) *d++ = *s++;
	return dst;
}

void *memsetGlibc (void *dst, int c, size_t n)
{
	return memset(dst, c, n);
}

void *memset8 (void *dst, int c, size_t n)
{
	u8	*d = dst;

	while (n--) *d++ = c;
	return dst;
}

void *memset32 (void *dst, int c, size_t n)
{
	u32	*d = dst;

	n /= sizeof(*d);
	while (n--) *d++ = c;
	return dst;
}

void *memset64 (void *dst, int c, size_t n)
{
	u64	*d = dst;
	u64	v = (((u64)c) << 32) | (u32)c;

	n /= sizeof(*d);
	while (n--) *d++ = v;
	return dst;
}

int summem8 (const void *src, size_t n)
{
	const u8	*s = src;
	int		sum = 0;

	while (n--) sum += *s++;
	return sum;
}

int summem32 (const void *src, size_t n)
{
	const u32	*s = src;
	int		sum = 0;

	n /= sizeof(*s);
	while (n--) sum += *s++;
	return sum;
}

int summem64 (const void *src, size_t n)
{
	const u64	*s = src;
	int		sum = 0;

	n /= sizeof(*s);
	while (n--) sum += *s++;
	return sum;
}


/* Sets to pseudo random values and insures each page is mapped */
void initMem (void *mem, int n)
{
	u64	*m = mem;
	u64	a = twister_random();

	n /= sizeof(u64);
	while (n-- != 0) {
		*m++ = a++;
	}
}

void memcpyTest (char *test_name, memcpy_f f)
{
	struct rusage before;
	struct rusage after;
	u64 start;
	u64 finish;
	u8 *a;
	u8 *b;
	int n;
	int i;
	int j;
	printf("%s (%s)\n", test_name, meg.legend);
	n = Option.file_size;
	a = alloc_aligned(n);
	b = alloc_aligned(n);
	if (init_buffers) {
		initMem(a, n);
		initMem(b, n);
	}
	for (j = 0; j < Option.loops; j++) {
		getrusage(RUSAGE_SELF, &before);
		start = nsecs();
		for (i = Option.iterations; i > 0; i--) {
			f(a, b, n);
			if (bidirectional) f(b, a, n);
		}
		finish = nsecs();
		if (bidirectional) {
			printf("%d. %g %s/sec\n", j,
				2.0 * meg.scale * (n * (u64)Option.iterations) /
					(double)(finish - start),
				meg.units);
		} else {
			printf("%d. %g %s/sec\n", j,
				meg.scale * (n * (u64)Option.iterations) /
					(double)(finish - start),
				meg.units);
		}
		getrusage(RUSAGE_SELF, &after);
		PrUsage(&before);
		PrUsage(&after);
	}
	free(a);
	free(b);
}

void memsetTest (char *test_name, memset_f f)
{
	struct rusage	before;
	struct rusage	after;
	u64	start;
	u64	finish;
	u8	*a;
	int	n;
	int	i;
	int	j;
	printf("%s (%s)\n", test_name, meg.legend);
	n = Option.file_size;
	a = alloc_aligned(n);
	if (init_buffers) {
		initMem(a, n);
	}
	for (j = 0; j < Option.loops; j++) {
		getrusage(RUSAGE_SELF, &before);
		start = nsecs();
		for (i = Option.iterations; i > 0; i--) {
			f(a, 0, n);
		}
		finish = nsecs();
		printf("%d. %g %s/sec\n", j,
			meg.scale * (n * (u64)Option.iterations) /
				(double)(finish - start),
			meg.units);
		getrusage(RUSAGE_SELF, &after);
		PrUsage(&before);
		PrUsage(&after);
	}
	free(a);
}

void summemTest (char *test_name, summem_f f)
{
	struct rusage	before;
	struct rusage	after;
	u64	start;
	u64	finish;
	u8	*a;
	int	n;
	int	i;
	int	j;
	printf("%s (%s)\n", test_name, meg.legend);
	n = Option.file_size;
	a = alloc_aligned(n);
	if (init_buffers) {
		initMem(a, n);
	}
	for (j = 0; j < Option.loops; j++) {
		getrusage(RUSAGE_SELF, &before);
		start = nsecs();
		for (i = Option.iterations; i > 0; i--) {
			f(a, n);
		}
		finish = nsecs();
		printf("%d. %g %s/sec\n", j,
			meg.scale * (n * (u64)Option.iterations) /
				(double)(finish - start),
			meg.units);
		getrusage(RUSAGE_SELF, &after);
		PrUsage(&before);
		PrUsage(&after);
	}
	free(a);
}

pthread_mutex_t StartLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t WaitLock = PTHREAD_MUTEX_INITIALIZER;
int Wait;

static void Ready (void)
{
	pthread_mutex_lock(&WaitLock);
	--Wait;
	pthread_mutex_unlock(&WaitLock);
	pthread_mutex_lock(&StartLock);
	pthread_mutex_unlock(&StartLock);
}

void *RunTest (void *arg)
{
	Ready();

	printf("memcpy tests:\n");
	memcpyTest("memcpy", memcpy);
	memcpyTest("simple", memcpySimple);
	memcpyTest("glibc", memcpyGlibc);
	memcpyTest("32bit", memcpy32);
	memcpyTest("64bit", memcpy64);

	printf("\nmemset tests:\n");
	memsetTest("memset", memset);
	memsetTest("8bit", memset8);
	memsetTest("glibc", memsetGlibc);
	memsetTest("32bit", memset32);
	memsetTest("64bit", memset64);

	printf("\nsummem tests:\n");
	summemTest("8bit", summem8);
	summemTest("32bit", summem32);
	summemTest("64bit", summem64);
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

bool myopt (int c)
{
	switch (c) {
	case 'b':
		bidirectional = FALSE;
		break;
	case 'm':
		meg.scale   = 1000.0;
		meg.units   = "MB";
		meg.legend  = "Meg = 10**6";
		break;
	case 'n':
		bidirectional = FALSE;
		init_buffers = FALSE;
		break;
	case 'u':
		resource_usage = TRUE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void usage (void)
{
	pr_usage("-bhmnu -i<iterations> -l<loops> -t<threads> -z<copy size>\n"
		"\tb - turn off bi-directional copy\n"
		"\th - help\n"
		"\ti - copy buffer i times [%lld]\n"
		"\tl - number of trials to run [%lld]\n"
		"\tm - use Meg == 10**6 [2**20]\n"
		"\tn - no initialization - for demonstrating shared pages\n"
		"\t\tAlso sets the -b option\n"
		"\tt - number of threads [%lld]\n"
		"\tu - print resource usage [off]\n"
		"\tz - size of copy buffer in bytes (can use hex) [0x%llx]",
		Option.iterations, Option.loops,
		Option.numthreads, Option.file_size);
}

int main (int argc, char *argv[])
{
	Option.iterations = 2;
	Option.loops = 4;
	Option.file_size = (1<<24);
	Option.numthreads = 1;
	punyopt(argc, argv, myopt, "bmnu");
	StartThreads();
	return 0;
}
