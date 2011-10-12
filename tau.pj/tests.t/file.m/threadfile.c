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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <eprintf.h>
#include <debug.h>
#include <mystdlib.h>
#include <puny.h>
#include <style.h>


enum { MAX_NAME = 15 };

typedef struct inst_s {
	u64	num_created;
	u64	num_deleted;
	u64	num_written;
	u64	num_read;
} inst_s;

typedef struct arg_s {
	unsigned	seed;
} arg_s;

struct {
	bool	rate;
} Myopt = {
	.rate  = FALSE };

inst_s Inst;

bool Stop = FALSE;

void pr_inst (inst_s *inst)
{
	printf("created = %10llu\n", inst->num_created);
	printf("deleted = %10llu\n", inst->num_deleted);
	printf("written = %10llu\n", inst->num_written);
	printf("read    = %10llu\n", inst->num_read);
}

void pr_delta (inst_s *d)
{
	printf("%10llu %10llu %10llu %10llu\n",
		d->num_created, d->num_deleted, d->num_written, d->num_read);
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

void cleanup (char *dir)
{
	cd("..");
	int rc = rmdir(dir);
	if (rc) fatal("rmdir %s:", dir);
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
	++Inst.num_created;
//	fill(fd, arg);
	close(fd);
	return 0;
}

void del_file (char *name)
{
	int	rc;

	rc = unlink(name);
	if (rc) {
		eprintf("unlink \"%s\" :", name);
		return;
	}
	++Inst.num_deleted;
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

pthread_spinlock_t Name_lock;

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

void init_name (void)
{
	Name = emalloc(Max * sizeof(char *));
	init_lock();
}

char *rand_remove_name (arg_s *arg)
{
	unint x;
	char *name;

	lock();
	if (Next) {
		x = urand_r(Next, &arg->seed);
		name = Name[x];
		Name[x] = Name[--Next];
	} else {
		return NULL;
	}
	unlock();
	return name;
}

char *remove_name (void)
{
	char *name;

	lock();
	if (Next) {
		name = Name[--Next];
	} else {
		name = NULL;
	}
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

void cleanup_files (void)
{
	char *name;
	for (;;) {
		name = remove_name();
		if (!name) break;
		del_file(name);
		free(name);
	}
}

bool should_delete (arg_s *arg)
{
	enum { RANGE = 1<<10, MASK = (2*RANGE) - 1 };
	return (rand_r(&arg->seed) & MASK) * Next / Level / RANGE;
}

void *crfiles (void *arg)
{
	char *name;
	int i;

	for (i = 0; i < Option.iterations; i++) {
		if (should_delete(arg)) {
			name = rand_remove_name(arg);
			if (!name) continue;
			del_file(name);
			free(name);
		} else {
			name = emalloc(MAX_NAME);
			gen_name(name, arg);
			cr_file(name, arg);
			add_name(name);
		}
	}
	cleanup_files();
	Stop = TRUE;
	return NULL;
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
		SET_DELTA(num_created);
		SET_DELTA(num_deleted);
		SET_DELTA(num_written);
		SET_DELTA(num_read);
		pr_delta( &delta);
		if (Stop) {
			zero(old);
			return;
		}
		old = new;
	}
}

void start_threads (unsigned threads)
{
	pthread_t	*thread;
	unsigned	i;
	int		rc;
	arg_s		*arg;
	arg_s		*a;

	Stop = FALSE;
	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		a->seed  = random();
		rc = pthread_create( &thread[i], NULL, crfiles, a);
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

void init (char *dir)
{
	int rc = mkdir(dir, 0700);
	if (rc) fatal("mkdir %s:", dir);
	cd(dir);
	init_name();
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
	default:
		return FALSE;
	}
	return TRUE;
}

int main (int argc, char *argv[])
{
	char		*dir;
	unsigned	threads;
	unsigned	i;

	Option.iterations = 2000;
	Option.loops = 3;
	Option.numthreads = 4;
	punyopt(argc, argv, myopt, "r");
	threads = Option.numthreads;
	dir = Option.dir;

	init(dir);

	for (i = 0; i < Option.loops; i++) {
		srandom(137);

		startTimer();
		start_threads(threads);
		stopTimer();

		pr_inst( &Inst);
		prTimer();
		printf("\n");
		clear_inst();
	}
	cleanup(dir);
	return 0;
}
