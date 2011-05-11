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
 * OPEN test
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <style.h>
#include <myio.h>
#include <mylib.h>
#include <eprintf.h>
#include <puny.h>

typedef struct arg_s {
	char		name[256];
	unsigned	n;
} arg_s;

void *do_opens (void *arg)
{
	arg_s	*a = arg;
	int	n = a->n;
	int	i;
	int	fd;

	fd = creat(a->name, 0755);
	if (fd == -1) eprintf("creat %s:", a->name);
	close(fd);

	for (i = 0; i < n; ++i) {
		fd = open(a->name, O_RDONLY);
		if (fd == -1) {
			perror("open");
			exit(1);
		}
		close(fd);
	}
	unlink(a->name);
	return NULL;
}

void start_threads (unsigned threads, unsigned n)
{
	pthread_t	*thread;
	unsigned	i;
	int		rc;
	arg_s		*arg;
	arg_s		*a;

	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		sprintf(a->name, "file_%d", i);
		a->n = n;
		rc = pthread_create( &thread[i], NULL, do_opens, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

void usage (void)
{
	pr_usage("-d<dir> -i<num opens> -t <threads> -l<loops>");
}

int main (int argc, char *argv[])
{
	unsigned	i;
	unsigned	threads;
	unsigned	n;

	punyopt(argc, argv, NULL, NULL);
	n = Option.iterations;
	threads = Option.numthreads;
	chdirq(Option.dir);
	for (i = 0; i < Option.loops; i++) {
		startTimer();
		start_threads(threads, n);
		stopTimer();
		prTimer();
		printf("\n");
	}
	return 0;
}

