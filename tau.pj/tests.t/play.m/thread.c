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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <debug.h>
#include <style.h>
#include <mystdlib.h>
#include <eprintf.h>

typedef struct thread_s {
	int		th_id;
	pthread_t	th_thread;
} thread_s;

void prompt (int x)
{
	printf("%d$ ", x);
	getchar();
}

void *start (void *thread)
{
	thread_s	*th = thread;

	prompt(th->th_id);

	return NULL;
}

void start_threads (unsigned threads)
{
	thread_s	*thread;
	thread_s	*th;
	unsigned	i;
	int		rc;

	thread = ezalloc(threads * sizeof(thread_s));
	for (i = 0; i < threads; i++) {
		th = &thread[i];
		th->th_id = i;
		rc = pthread_create( &th->th_thread, NULL, start, th);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	while (i--) {
		pthread_join(thread[i].th_thread, NULL);
	}
}

void usage (void)
{
	printf("Usage: %s [threads]\n",
		getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	unsigned	threads = 11;

	setprogname(argv[0]);
	if (argc > 1) {
		threads = atoi(argv[1]);
	}
	//seed_random();

	if (!threads) {
		usage();
	}

	startTimer();
	start_threads(threads);
	stopTimer();

	prTimer();

	printf("\n");

	return 0;
}
