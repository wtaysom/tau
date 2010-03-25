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

#define VOLATILE	1
#define SLEEP		0
#define DONOTHING	0

unint	Loops = 100000;

enum {	RING_SIZE = 1<<5,
	RING_MASK = RING_SIZE - 1 };

#if VOLATILE
typedef struct ring_s {
	volatile unint	r_give;
	unint		r_ring[RING_SIZE];
	volatile unint	r_take;
} ring_s;
#else
typedef struct ring_s {
	unint	r_give;
	unint	r_ring[RING_SIZE];
	unint	r_take;
} ring_s;
#endif

ring_s	Ring;

void *producer (void *arg)
{
	u64	start, finish;
	unint	wait = 0;
	unint	i;

printf("Producer\n");
	start = nsecs();
	for (i = 0; i < Loops; i++) {
		while (((Ring.r_give + 1) & RING_MASK) == Ring.r_take) {
			#if SLEEP
				sleep(0);
			#elif DONOTHING
				donothing();
			#endif
			++wait;
		}
		Ring.r_ring[Ring.r_give] = i;
		if (Ring.r_give == RING_MASK) {
			Ring.r_give = 0;
		} else {
			++Ring.r_give;
		}
	}
	finish = nsecs();
	printf("producer %g nsecs per increment and %g waits\n",
		((double)(finish - start)) / Loops, (double)wait / Loops);
	return NULL;
}

void *consumer (void *arg)
{
	u64	start, finish;
	unint	wait = 0;
	unint	i;

printf("Consumer\n");
	start = nsecs();
	for (i = 0; i < Loops; i++) {
		while (Ring.r_take == Ring.r_give) {
			#if SLEEP
				sleep(0);
			#elif DONOTHING
				donothing();
			#endif
			++wait;
		}
		if (Ring.r_ring[Ring.r_take] != i) {
			fatal("%ld != %ld", Ring.r_ring[Ring.r_take], i);
		}
		if (Ring.r_take == RING_MASK) {
			Ring.r_take = 0;
		} else {
			++Ring.r_take;
		}
	}
	finish = nsecs();
	printf("consumer %g nsecs per increment and %g waits\n",
		((double)(finish - start)) / Loops, (double)wait / Loops);
	return NULL;
}

void start_threads (void)
{
	pthread_t	prod;
	pthread_t	con;
	int		rc;

	rc = pthread_create( &prod, NULL, producer, NULL);
	if (rc) {
		fatal("couldn't create producer thread %d:", rc);
	}
	rc = pthread_create( &con, NULL, consumer, NULL);
	if (rc) {
		fatal("couldn't create consumer thread %d:", rc);
	}
	pthread_join(prod, NULL);
	pthread_join(con, NULL);
}

void usage (void)
{
	printf("Usage: %s [loops]\n", getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	setprogname(argv[0]);
	if (argc > 1) {
		Loops = atoi(argv[1]);
	}
	if (argc > 2) {
		usage();
	}

	start_threads();

	return 0;
}
