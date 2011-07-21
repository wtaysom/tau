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
#include <mystdlib.h>
#include <eprintf.h>
#include <debug.h>
#include <timer.h>

typedef struct arg_s {
	char	name[128];
} arg_s;

unint	Loops = 100000;
#if 1
unint	P;
//unint	Array[20000];
unint	C;
#else
volatile unint	P;
//volatile unint	Array[20000];
volatile unint	C;
#endif

#define SLEEP 0
#define DONOTHING 1

void *producer (void *arg)
{
	u64	start, finish;
	unint	wait = 0;
	unint	i;

	++P;
	while (P > C) {
		sleep(0);
	}
printf("Producer\n");
	start = nsecs();
	for (i = 0; i < Loops; i++, P++) {
		while (P > C) {
			#if SLEEP
				sleep(0);
			#elif DONOTHING
				donothing();
			#endif
			++wait;
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
	for (i = 0; i < Loops; i++, C++) {
		while (C >= P) {
			#if SLEEP
				sleep(0);
			#elif DONOTHING
				donothing();
			#endif
			++wait;
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
