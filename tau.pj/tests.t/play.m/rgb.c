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
 * rgb, red-green-blue, is a simple benchmark for locks. Time measurements
 * are done in clock cycles.
 *
 * Two threads contend for the three locks, red, green, and blue, in
 * the following order;
 *
 *	Thread 1	Thread 2
 *	lock	red	lock	red
 *	get	red	wait	red
 *	lock	green	wait	red
 *	get	green	wait	red
 *	unlock	red	get	red
 *	lock	blue	lock	green
 *      get	blue	wait	green
 *	unlock	green	get	green
 *	lock	red	unlock	red
 *	get	red	lock	blue
 *	unlock	blue	get	blue
 *	lock	green	unlock	green
 *	get	green	lock	red
 *	unlock	red	get	red
 *	lock	blue	unlock	blue
 *	get	blue	lock	green
 *	unlock	green	get	green
 *	lock	red	unlock	red
 *	get	red	lock	blue
 *	unlock	blue	get	blue
 *	lock	green	unlock	green
 *	get	green	lock	red
 *	unlock	red	get	red
 *	lock	blue	unlock	blue
 *	get	blue	lock	green
 *	unlock	green	get	green
 *	   .		   .
 *	   .		   .
 *	   .		   .
 *
 * To configure, set defines below:
 *	DEBUG	runs on extra printing to debug rgb
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
#else
	#define DEBUG	0
	#define SPIN	0
	#define MUTEX	1
	#define TSPIN	0
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
#include <timer.h>

typedef struct arg_s {
	char	name[128];
} arg_s;

typedef struct rgb_s {
	char		*name;
#if MUTEX
	pthread_mutex_t	lock;
#elif SPIN
	raw_spinlock_t	lock;
#elif TSPIN
	pthread_spinlock_t	lock;
#else
	unint		lock;
#endif
//	unint		padding[4096];
} rgb_s;

#if MUTEX
	#define INIT_LOCK	PTHREAD_MUTEX_INITIALIZER
#elif SPIN
	#define INIT_LOCK	__RAW_SPIN_LOCK_UNLOCKED
#elif TSPIN
	#define INIT_LOCK	0
#else
	#define INIT_LOCK	0
#endif

rgb_s	Red   = { "red",   INIT_LOCK};
rgb_s	Green = { "green", INIT_LOCK};
rgb_s	Blue  = { "blue",  INIT_LOCK};

unint	Loops = 100000;

static inline void rgb_lock (char *name, rgb_s *lock)
{
#if DEBUG
	printf("%s lock   %s\n", name, lock->name);
#endif
#if MUTEX
	pthread_mutex_lock( &lock->lock);
#elif SPIN
	__raw_spin_lock( &lock->lock);
#elif TSPIN
	pthread_spin_lock( &lock->lock);
#endif
}

static inline void rgb_unlock (char *name, rgb_s *lock)
{
#if DEBUG
	printf("%s unlock %s\n", name, lock->name);
#endif
#if MUTEX
	pthread_mutex_unlock( &lock->lock);
#elif SPIN
	__raw_spin_unlock( &lock->lock);
#elif TSPIN
	pthread_spin_unlock( &lock->lock);
#endif
}

void *rgb (void *arg)
{
	arg_s	*a = arg;
	u64	start, finish;
	unint	i;

	rgb_lock(a->name, &Red);
	start = nsecs();
	for (i = Loops; i; i--) {
#if (!DEBUG && !MUTEX && !SPIN && !TSPIN)
		donothing();
#endif
		rgb_lock(a->name, &Green);
		rgb_unlock(a->name, &Red);
		rgb_lock(a->name, &Blue);
		rgb_unlock(a->name, &Green);
		rgb_lock(a->name, &Red);
		rgb_unlock(a->name, &Blue);
	}
	rgb_unlock(a->name, &Red);
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

	rgb_lock("main", &Red);
	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		sprintf(a->name, "rgb%d", i);
		rc = pthread_create( &thread[i], NULL, rgb, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	sleep(1);
	rgb_unlock("main", &Red);
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

void init (void)
{
#if TSPIN
	pthread_spin_init( &Red.lock,   PTHREAD_PROCESS_PRIVATE);
	pthread_spin_init( &Green.lock, PTHREAD_PROCESS_PRIVATE);
	pthread_spin_init( &Blue.lock,  PTHREAD_PROCESS_PRIVATE);
#endif
}

void usage (void)
{
	printf("Usage: %s [loops [threads]]\n", getprogname());
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
		threads = atoi(argv[2]);
	}
	if (argc > 3) {
		usage();
	}
	if (!threads) {
		usage();
	}

	init();

#if MUTEX
	printf("pthread mutex\n");
#elif SPIN
	printf("raw spinlock\n");
#elif TSPIN
	printf("pthread spinlock\n");
#else
	printf("Empty test\n");
#endif

	start_threads(threads);

	return 0;
}
