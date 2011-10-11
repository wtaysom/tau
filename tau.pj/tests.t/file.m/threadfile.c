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
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <spinlock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <eprintf.h>
#include <debug.h>
#include <style.h>
#include <mystdlib.h>
#include <puny.h>


enum { MAX_NAME = 15 };

typedef struct inst_s {
	u64	num_created;
	u64	num_deleted;
} inst_s;

typedef struct arg_s {
	unsigned	seed;
} arg_s;

struct {
	bool	rate;
} Myopt = {
	.rate  = FALSE };

inst_s Inst;

void pr_inst (inst_s *inst)
{
	printf("created = %10llu\n", inst->num_created);
	printf("deleted = %10llu\n", inst->num_deleted);
}

void pr_delta (inst_s *d)
{
	printf("%10llu %10llu %10llu %10llu %10llu %10llu %10llu\n",
		d->num_opens, d->num_dirs+d->num_files, d->num_dirs,
		d->num_files, d->num_deleted, d->num_read, d->num_written);
}

void clear_inst (void)
{
	zero(Inst);
}

void gen_name (char *c, arg_s *arg)
{
	unsigned	i;
	static char file_name_char[] =  "abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"_0123456789";

	for (i = 0; i < MAX_NAME - 1; i++) {
		*c++ = file_name_char[urand_r(sizeof(file_name_char)-1,
					&arg->seed)];
	}
	*c = '\0';
}

void cd (char *dir)
{
	int	rc;

	rc = chdir(dir);
	if (rc){
		eprintf("chdir \"%s\" :", dir);
	}
}

void init (char *dir)
{
	int rc = mkdir(dir, 0700);
	if (rc) fatal("mkdir %s:", dir);
	cd(dir);
}

void cleanup (char *dir)
{
	cd("..");
	int rc = rmdir(dir);
	if (rc) fatal("rmdir %s:", dir);
}

int open_file (char *name)
{
	int	fd;

	fd = open(name, O_RDWR);
	if (fd == -1) {
		eprintf("open_file \"%s\" :", name);
		return fd;
	}
	++Inst.num_opens;
	return fd;
}

void fill (int fd, arg_s *arg)
{
	static char	rnd_char[] = "abcdefghijklmnopqrstuvwxyz\n";
	char		buf[4096];
	unsigned long	i, n;

	n = (urand_r(10, &arg->seed)+1)
	  * (urand_r(10, &arg->seed)+1)
	  * (urand_r(10, &arg->seed)+1);

	for (i = 0; i <  sizeof(buf); i++) {
		buf[i] = rnd_char[urand_r(sizeof(rnd_char)-1, &arg->seed)];
	}
	for (i = 0; i < n; i++) {
		if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
			fatal("write:");
		}
	}
	Inst.num_written += n * sizeof(buf);
}

int cr_file (char *name, arg_s *arg)
{
	int	fd;

	fd = creat(name, 0666);
	if (fd == -1) {
		eprintf("cr_file \"%s\" :", name);
		return -1;
	}
	++Inst.num_files;
	fill(fd, arg);
	close(fd);
	return 0;
}

void del_file (char *name)
{
	int	rc;

	rc = unlink(name);
	if (rc) {
		eprintf("del_file \"%s\" :", name);
		return;
	}
	++Inst.num_deleted;
}

void rand_file (char *path, arg_s *arg)
{
	char	name[MAX_NAME];

	gen_name(name, arg);

	add_name(path, name);
	cr_file(path, arg);
	drop_name(path);
}

unint Level = 100;
unint Max = 200;
unint Next = 0;
char **Name;

#ifdef __APPLE__

pthread_mutex_t Name_lock;

static void init_lock (void)
{
	pthread_mutext_init( &Name_lock, NULL)
}

static void lock (void)
{
	pthread_mutex_lock(&Name_lock);
}

void unlock (void)
{
	pthread_unmutex_lock(&Name_lock);
}

#else

pthread_spinlock_t Lock_name;

void init_lock (void)
{
	pthread_spin_init( &Name_lock, PTHREAD_PROCESS_PRIVATE);
}

void lock (void)
{
	pthread_spin_lock(&Name_lock);
}

void unlock (void)
{
	pthread_spin_unlock(&Name_lock);
}

#endif

char *rand_remove_name (arg_s *arg)
{
	unint x;
	char *name;

	lock();
	if (!Next) return NULL;

	x = urand_r(Next, &arg->seed);
	name = Name[x];
	Name[x] = Name[--Next];

	unlock();
	return name;
}

char *remove_name (void)
{
	char *name;

	lock();
	if (!Next) return NULL;

	name = Name[--Next];

	unlock();
	return name;
}

void add_name (char *name)
{
	lock();

	if (Next == Max) fatal("Next %ld = Max %ld Level = %ldfor %s",
				Next, Max, Level, name);
	Name[Next++] = name;

	unlock();
}

bool should_delete (args_s *arg)
{
	enum { RANGE = 1<<10, MASK = (2*RANGE) - 1 };
	return (rand_r(&arg->seed) & MASK) * Next / Level / RANGE;
}

void *crfiles (void *arg)
{
	arg_s *a = arg;
	char *name;
	int rc;

	for (;;) {
		if (should_delete(Next, Level)) {
			name = rand_remove_name(arg);
			rc = unlink(name);
			if (rc != 0) fatal("unlink %s:", name);
			free(name);
		} else {
			name = emalloc(MAX_NAME);
			gen_name(name, arg);
			cr_file(name, arg);
			add_name(name);
		}
	}
}

void rate (void)
{
#define SET_DELTA(x)	delta.x = new.x - old.x
	static inst_s	old = { 0 };
	static inst_s	new;
	static inst_s	delta;

	for (;;) {
		sleep(1);
		new = Inst;
		SET_DELTA(num_opens);
		SET_DELTA(num_dirs);
		SET_DELTA(num_files);
		SET_DELTA(num_deleted);
		SET_DELTA(num_read);
		SET_DELTA(num_written);
		pr_delta( &delta);
		if (delta.num_opens+delta.num_dirs+delta.num_files+
		    delta.num_deleted+delta.num_read+
		    delta.num_written == 0) {
			zero(old);
			return;
		}
		old = new;
	}
}

void start_threads (unsigned threads, unsigned width, unsigned depth, char *from, char *to)
{
	pthread_t	*thread;
	unsigned	i;
	int		rc;
	arg_s		*arg;
	arg_s		*a;

	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		sprintf(a->from, "%s_%d", from, i);
		sprintf(a->to,   "%s_%d", to, i);
		a->width = width;
		a->depth = depth;
		a->seed  = random();
		rc = pthread_create( &thread[i], NULL, ztree, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	if (Myopt.rate) rate();
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

void usage (void)
{
	pr_usage("-r -i<iterations> -t<threads> -w<width> -k<depth>"
		" -d<start> -f<from> -o<to>");
}

bool myopt (int c)
{
	switch (c) {
	case 'r':
		Myopt.rate = TRUE;
		break;
	case 'k':
		Myopt.depth = strtoll(optarg, NULL, 0);
		break;
	case 'w':
		Myopt.width = strtoll(optarg, NULL, 0);
		break;
	case 'f':
		Myopt.from = optarg;
		break;
	case 'o':
		Myopt.to = optarg;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int main (int argc, char *argv[])
{
	char		*dir;
	char		*from;
	char		*to;
	unsigned	threads;
	unsigned	width;
	unsigned	depth;
	unsigned	i;

	Option.iterations = 4;
	Option.numthreads = 2;
	punyopt(argc, argv, myopt, "rk:w:f:o:");
	threads = Option.numthreads;
	dir     = Option.dir;
	width   = Myopt.width;
	depth   = Myopt.depth;
	from    = Myopt.from;
	to      = Myopt.to;

	init(dir);

	for (i = 0; i < Option.iterations; i++) {
		srandom(137);

		startTimer();
		start_threads(threads, width, depth, from, to);
		stopTimer();

		pr_inst( &Inst);

		prTimer();

		printf("\n");

		clear_inst();
	}
	cleanup(dir);
	return 0;
}
