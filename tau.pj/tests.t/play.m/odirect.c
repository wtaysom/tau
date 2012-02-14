/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Playing with the O_DIRECT flag. I want to see if it blocks other processes
 * from getting at the disk.
 *
 * 	"The thing that has always disturbed me about O_DIRECT is that  the
 *	whole  interface  is  just  stupid,  and was probably designed by a
 *	deranged monkey on some  serious  mind-controlling  substances."  —
 *	Linus
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <puny.h>
#include <timer.h>

enum { MAX_NAME = 100 };

typedef struct Buf_s {
	unint	size;
	u8	*data;
} Buf_s;

typedef struct Args_s {
	unint	id;
} Args_s;

void pr_time (u64 ns)
{
	static unint	i = 0;
	char	*units;
	u64	scale;

	if (ns > ONE_BILLION) {
		scale = ONE_BILLION;
		units = "s";
	} else if (ns > ONE_MILLION) {
		scale = ONE_MILLION;
		units = "ms";
	} else if (ns > ONE_THOUSAND) {
		scale = ONE_THOUSAND;
		units = "us";
	} else {
		scale = 1;
		units = "ns";
	}
	printf("%4ld. write took %6.1f %s\n", ++i, (double)ns / scale, units);
}

Buf_s prepare_buf (void)
{
	enum { PAGE_SIZE = 1<<12 };
	Buf_s	buf;
	unint	i;

	buf.data = eallocpages(Option.file_size, PAGE_SIZE);
	for (i = 0; i < Option.file_size; i++) {
		buf.data[i] = i % 107;
	}
	buf.size = Option.file_size * PAGE_SIZE;
	return buf;
}

void* writer (void *arg)
{
	struct timespec sleep = { Option.sleep_secs, 0 * ONE_MILLION };
	Args_s	*a = arg;
	char	file[MAX_NAME];
	int	fd;
	int	rc;
	Buf_s	buf;
	unint	loops;
	unint	i;

	snprintf(file, MAX_NAME-1, "file_%ld", a->id);
	buf = prepare_buf();
	for (loops = 0; loops < Option.loops; loops++) {
#ifdef __APPLE__
		fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0700);
		if (fd == -1) fatal("open %s:", file);
		rc = fcntl(fd, F_NOCACHE);
		if (rc == -1) fatal("fcntl %s:", file);
#else
		fd = open(file, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, 0700);
		if (fd == -1) fatal("open %s:", file);
#endif
		unlink(file);
		nanosleep(&sleep, NULL);
		u64 start = nsecs();
		for (i = 0; i < Option.iterations; i++) {
			rc = pwrite(fd, buf.data, buf.size, 0);
			if (rc == -1) fatal("unexpected write error %s:", file);
		}
		u64 finish = nsecs();
		close(fd);
		pr_time(finish - start);
	}
	free(buf.data);
	return NULL;
}

void start_threads (void)
{
	pthread_t	*thread;
	unsigned	i;
	unsigned	n = Option.numthreads;
	int		rc;
	Args_s		*args;
	Args_s		*a;

	thread = ezalloc(n * sizeof(pthread_t));
	args   = ezalloc(n * sizeof(Args_s));
	for (i = 0, a = args; i < n; i++, a++) {
		a->id = i;
		rc = pthread_create(&thread[i], NULL, writer, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

void cd (char *dir)
{
	int	rc;

	rc = chdir(dir);
	if (rc){
		eprintf("chdir \"%s\" :", dir);
	}
}

void cleanup (char *dir)
{
	cd("..");
	int rc = rmdir(dir);
	if (rc) fatal("rmdir %s:", dir);
}

void fatal_cleanup (void)
{
	static bool	cleaning_up = FALSE;

	char	cmd[1024];
	int	rc;

	if (!Option.cleanup || cleaning_up) return;
	cleaning_up = TRUE;
	cd("..");
	rc = snprintf(cmd, sizeof(cmd), "rm -fr %s", Option.dir);
	if (rc > sizeof(cmd) - 2) return;	/* shouldn't be that big */
	rc = system(cmd);
	if (rc) {
		fatal("'%s' failed %d:", cmd, rc);
	}
}

void init (char *dir)
{
	int rc = mkdir(dir, 0700);
	if (rc) fatal("mkdir %s:", dir);
	cd(dir);
	set_cleanup(fatal_cleanup);
}

void usage (void)
{
	pr_usage("-d<directory> -i<iterations> -l<loops> -s<secs to sleep>"
		" -t<num threads> -z<buf size in pages\n"
		"\td - directory for where to create files [%s]\n"
		"\ti - iterations [%lld]\n"
		"\tl - loops [%lld]\n"
		"\ts - seconds to sleep between loops [%lld]\n"
		"\tt - number of threads [%lld]\n"
		"\tz - buf size in pages [%lld]",
		Option.dir, Option.iterations, Option.loops, Option.sleep_secs,
		Option.numthreads, Option.file_size);
}

int main (int argc, char *argv[])
{
	Option.iterations = 100;
	Option.loops = 10;
	Option.file_size = 1000;
	punyopt(argc, argv, NULL, NULL);

	init(Option.dir);
	start_threads();	
	cleanup(Option.dir);
	return 0;
}
