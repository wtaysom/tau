/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

/*
 * Randomly lock and unlock a set of locks (only one at a time)
 *
 * To configure, set defines below:
 *	DEBUG	runs on extra printing to debug randlock
 *	SPIN	use raw spinlocks; code taken from kernel
 *	MUTEX	use thread mutex locks
 *	TSPIN	use thread spin locks
 * Only one of the last three should be set. Setting them all to
 * zero can be used to measure the overhead of the loop.
 */

#ifdef __linux__
	#define DEBUG	0
	#define SPIN	1
	#define MUTEX	0
	#define TSPIN	0
	#define GLOBAL	0
#else
	#define DEBUG	0
	#define SPIN	0
	#define MUTEX	1
	#define TSPIN	0
	#define GLOBAL	0
#endif

#if SPIN
	#include <spinlock.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include <style.h>
#include <mylib.h>
#include <eprintf.h>
#include <debug.h>
#include <timer.h>

typedef struct arg_s {
	char	name[128];
} arg_s;

typedef struct lock_s {
#if MUTEX
	pthread_mutex_t	lock;
#elif SPIN
	raw_spinlock_t	lock;
#elif TSPIN
	pthread_spinlock_t	lock;
#elif GLOBAL
	unint		lock;
#else
	unint		lock;
#endif
//	unint		padding[4096];
} lock_s;

#if MUTEX
	#define INIT_LOCK	PTHREAD_MUTEX_INITIALIZER
#elif SPIN
	#define INIT_LOCK	__RAW_SPIN_LOCK_UNLOCKED
#elif TSPIN
	#define INIT_LOCK	0
#elif GLOBAL
	#define INIT_LOCK	0
#else
	#define INIT_LOCK	0
#endif

#if SPIN
raw_spinlock_t	Init_spin_lock = __RAW_SPIN_LOCK_UNLOCKED;
#endif

unint	Loops = 10000;
unint	Num_locks = 10;
lock_s	*Locks;
pthread_mutex_t	Start = PTHREAD_MUTEX_INITIALIZER;

static void init_locks (unint n)
{
	lock_s	*lock;

	Locks = ezalloc(n * sizeof(lock_s));

	for (lock = Locks; lock < &Locks[n]; lock++) {
#if MUTEX
		pthread_mutex_init( &lock->lock, NULL);
#elif SPIN
		lock->lock = Init_spin_lock;
#elif TSPIN
		pthread_spin_init( &lock->lock, PTHREAD_PROCESS_PRIVATE);
#elif GLOBAL
		lock->lock = 0;
#endif
	}
}

static inline void lock (char *name, lock_s *lock)
{
#if DEBUG
	printf("%s lock   %p\n", name, lock);
#endif
#if MUTEX
	pthread_mutex_lock( &lock->lock);
#elif SPIN
	__raw_spin_lock( &lock->lock);
#elif TSPIN
	pthread_spin_lock( &lock->lock);
#elif GLOBAL
	++lock->lock;
#endif
}

static inline void unlock (char *name, lock_s *lock)
{
#if DEBUG
	printf("%s unlock %p\n", name, lock);
#endif
#if MUTEX
	pthread_mutex_unlock( &lock->lock);
#elif SPIN
	__raw_spin_unlock( &lock->lock);
#elif TSPIN
	pthread_spin_unlock( &lock->lock);
#elif GLOBAL
	--lock->lock;
#endif
}

static inline int rand_range (unint *seed, unint bound)
{
	*seed = *seed * 1103515245 + 12345;
	return *seed % bound;
}

void *randlock (void *arg)
{
	arg_s	*a = arg;
	unint	seed = random();
	u64	start, finish;
	unint	i;
	unint	k;

	pthread_mutex_lock( &Start);
	pthread_mutex_unlock( &Start);
	start = nsecs();
	for (i = Loops; i; i--) {
		k = rand_range( &seed, Num_locks);
#if (!DEBUG && !MUTEX && !SPIN && !TSPIN)
		donothing();
#endif
		lock(a->name, &Locks[k]);
		unlock(a->name, &Locks[k]);
	}
	finish = nsecs();
	printf("%s %g nsecs per lock-unlock pair\n", a->name,
		((double)(finish - start))/(3 * Loops));
	return NULL;
}

void start_threads (unsigned threads)
{
	pthread_t	*thread;
	unsigned	i;
	int		rc;
	arg_s		*arg;
	arg_s		*a;

	pthread_mutex_lock( &Start);
	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		sprintf(a->name, "lock_%d", i);
		rc = pthread_create( &thread[i], NULL, randlock, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	sleep(1);
	pthread_mutex_unlock( &Start);
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

void usage (void)
{
	printf("Usage: %s [loops [num_locks [threads]]]\n", getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	unsigned	threads = 2;

	setprogname(argv[0]);
	if (argc > 1) {
		Loops = atoi(argv[1]);
	}
	if (argc > 2) {
		Num_locks = atoi(argv[2]);
	}
	if (argc > 3) {
		threads = atoi(argv[3]);
	}
	if (argc > 4) {
		usage();
	}
	if (!threads) {
		usage();
	}

	init_locks(Num_locks);

#if MUTEX
	printf("pthread mutex\n");
#elif SPIN
	printf("raw spinlock\n");
#elif TSPIN
	printf("pthread spinlock\n");
#elif GLOBAL
	printf("global variable\n");
#else
	printf("Empty test\n");
#endif

	start_threads(threads);

	return 0;
}
