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

#include <debug.h>
#include <style.h>
#include <mylib.h>
#include <eprintf.h>
#include <puny.h>


enum { MAX_PATH = 1024, MAX_NAME = 8 };

typedef struct inst_s {
	u64	num_opens;
	u64	num_dirs;
	u64	num_files;
	u64	num_deleted;
	u64	num_read;
	u64	num_written;
} inst_s;

typedef struct arg_s {
	char		from[256];
	char		to[256];
	unsigned	width;
	unsigned	depth;
	unsigned	seed;
} arg_s;

inst_s Inst;
bool	Done;

void pr_inst (inst_s *inst)
{
	printf("opens   = %10llu\n", inst->num_opens);
	printf("created = %10llu\n", inst->num_dirs+inst->num_files);
	printf("dirs    = %10llu\n", inst->num_dirs);
	printf("files   = %10llu\n", inst->num_files);
	printf("deleted = %10llu\n", inst->num_deleted);
	printf("read    = %10llu\n", inst->num_read);
	printf("written = %10llu\n", inst->num_written);
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

void prompt (char *p)
{
	printf("%s$ ", p);
	getchar();
}

void add_name (char *parent, char *child)
{
	char	*c;

	c = &parent[strlen(parent)];
	cat(c, "/", child, NULL);
}

void drop_name (char *path)
{
	char	*c;

	for (c = &path[strlen(path)]; c != path; c--) {
		if (*c == '/') {
			break;
		}
	}
	*c = '\0';
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

int is_dir (const char *dir)
{
	struct stat	stbuf;
	int		rc;

	rc = stat(dir, &stbuf);
	if (rc) {
		eprintf("is_dir stat \"%s\" :", dir);
		return 0;
	}
	return S_ISDIR(stbuf.st_mode);
}

void cd (char *dir)
{
	int	rc;

	rc = chdir(dir);
	if (rc){
		eprintf("chdir \"%s\" :", dir);
	}
}

void init (char *working_directory)
{
	cd(working_directory);
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

DIR *open_dir (char *name)
{
	DIR	*dir;

	dir = opendir(name);
	if (!dir) {
		eprintf("open_dir \"%s\" :", name);
		return NULL;
	}
	++Inst.num_opens;
	return dir;
}

void close_dir (DIR *dir)
{
	closedir(dir);
}

void fill (int fd, arg_s *arg)
{
	static char	rnd_char[] = "abcdefghijklmnopqrstuvwxyz\n";
	char		buf[4096];
	unsigned long	i, n;

	n = urand_r(4, &arg->seed)+1;

	for (i = 0; i <  sizeof(buf); i++) {
		buf[i] = rnd_char[urand_r(sizeof(rnd_char)-1, &arg->seed)];
	}
	for (i = 0; i < n; i++) {
		if (write(fd, buf, sizeof(buf)) < 0) {
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
//	fill(fd, arg);
	close(fd);
	return 0;
}

int copy_file (char *from, char *to)
{
	char	buf[4096];
	int	from_fd;
	int	to_fd;
	int	n;
	int	rc;

	from_fd = open_file(from);
	if (from_fd == -1) {
		eprintf("copy_file open \"%s\" :", from);
		return -1;
	}
	to_fd = creat(to, 0666);
	if (to_fd == -1) {
		eprintf("copy_file creat \"%s\" :", to);
		return -1;
	}
	for (;;) {
		n = read(from_fd, buf, sizeof(buf));
		if (n == 0) break;

		if (n == -1) {
			eprintf("copy_file read \"%s\" :", from);
			return -1;
		}
		Inst.num_read += n;
		rc = write(to_fd, buf, n);
		if (rc != n) {
			eprintf("copy_file write \"%s\" :", to);
			return -1;
		}
		Inst.num_written += n;
	}
	close(to_fd);
	close(from_fd);
	return 0;
}

void cr_dir (char *name)
{
	int	rc;

	rc = mkdir(name, 0777);
	if (rc) {
		eprintf("cr_dir \"%s\" :", name);
		return;
	}
	++Inst.num_dirs;
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

void del_dir (char *name)
{
	int	rc;

	rc = rmdir(name);
	if (rc) {
		eprintf("del_dir \"%s\" :", name);
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

void rand_dir (char *path, arg_s *arg)
{
	char	name[MAX_NAME];

	gen_name(name, arg);

	add_name(path, name);
	cr_dir(path);
}

void mk_dir (char *path, unsigned width, unsigned depth, arg_s *arg)
{
	unsigned	i;

	for (i = 0; i < width; i++) {
		if (depth) {
			rand_dir(path, arg);
			mk_dir(path, width, depth-1, arg);
			drop_name(path);
		} else {
			rand_file(path, arg);
		}
	}
}

void mk_tree (char *name, int width, int depth, arg_s *arg)
{
	char	path[MAX_PATH];

	strcpy(path, name);
	cr_dir(path);
	mk_dir(path, width, depth, arg);
}

static void pr_indent (unsigned level)
{
	while (level--) {
		printf("  ");
	}
}

void walk_dir (char *path, unsigned level)
{
	struct dirent	*d;
	DIR		*dir;

	dir = open_dir(path);
	for (;;) {
		d = readdir(dir);
		if (!d) break;
		if ((strcmp(d->d_name, ".")  == 0) ||
		    (strcmp(d->d_name, "..") == 0)) {
			continue;
		}
		pr_indent(level); printf("%s\n", d->d_name);
		add_name(path, d->d_name);
		if (is_dir(path)) {
			walk_dir(path, level+1);
		}
		drop_name(path);
	}
	close_dir(dir);
}

void walk_tree (char *name)
{
	char	path[MAX_PATH];

	strcpy(path, name);
	walk_dir(path, 0);
}

void copy_dir (char *from, char *to)
{
	struct dirent	*d;
	DIR		*dir;

	dir = open_dir(from);
	cr_dir(to);
	for (;;) {
		d = readdir(dir);
		if (!d) break;
		if ((strcmp(d->d_name, ".")  == 0) ||
		    (strcmp(d->d_name, "..") == 0)) {
			continue;
		}
		add_name(to, d->d_name);
		add_name(from, d->d_name);
		if (is_dir(from)) {
			copy_dir(from, to);
		} else {
			copy_file(from, to);
		}
		drop_name(from);
		drop_name(to);
	}
	close_dir(dir);
}

void copy_tree (char *from, char *to)
{
	char	path_from[MAX_PATH];
	char	path_to[MAX_PATH];

	strcpy(path_from, from);
	strcpy(path_to, to);

	copy_dir(path_from, path_to);
}

void delete_dir (char *path)
{
	struct dirent	*d;
	DIR		*dir;

	dir = open_dir(path);
	for (;;) {
		d = readdir(dir);
		if (!d) break;
		if ((strcmp(d->d_name, ".")  == 0) ||
		    (strcmp(d->d_name, "..") == 0)) {
			continue;
		}
		add_name(path, d->d_name);
		if (is_dir(path)) {
			delete_dir(path);
		} else {
			del_file(path);
		}
		drop_name(path);
	}
	close_dir(dir);
	del_dir(path);
}

void delete_tree (char *name)
{
	char	path[MAX_PATH];

	strcpy(path, name);
	delete_dir(path);
}

void *ztree (void *arg)
{
	arg_s	*a = arg;

//printf("Begin mk_tree %s\n", a->from);
	mk_tree(a->from, a->width, a->depth, arg);
//	walk_tree(a->from);

//printf("Begin copy_tree %s\n", a->to);
//	copy_tree(a->from, a->to);
//	walk_tree(a->to);
#if 1
//printf("Begin delete_tree %s\n", a->from);
	delete_tree(a->from);
//printf("Begin delete_tree %s\n", a->to);
//	delete_tree(a->to);
#endif
	Done = TRUE;
	return NULL;
}

void rate (void)
{
#define DELTA(x)	delta.x = new.x - old.x
	static inst_s	old = { 0 };
	static inst_s	new;
	static inst_s	delta;

	for (;;) {
		sleep(1);
		new = Inst;
		DELTA(num_opens);
		DELTA(num_dirs);
		DELTA(num_files);
		DELTA(num_deleted);
		DELTA(num_read);
		DELTA(num_written);
		pr_delta( &delta);
		if (Done) {
			zero(old);
			return;
		}
		old = new;
	}
}

void start_threads (
	unsigned threads,
	unsigned width,
	unsigned depth,
	char *from,
	char *to)
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
	rate();
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

void usage (void)
{
	pr_usage("-i<iterations> -t<threads> -w<width> -k<depth>"
		" -d<start> -r<from> -o<to>");
}

struct {
	unsigned	width;
	unsigned	depth;
	char		*from;
	char		*to;
} Myopt = {
	.width = 2,
	.depth = 3,
	.from  = "ztree",
	.to    = "copy" };

bool myopt (int c)
{
	switch (c) {
	case 'k':
		Myopt.depth = strtoll(optarg, NULL, 0);
		break;
	case 'w':
		Myopt.width = strtoll(optarg, NULL, 0);
		break;
	case 'r':
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
	char		*start;
	char		*from;
	char		*to;
	unsigned	threads;
	unsigned	width;
	unsigned	depth;
	unsigned	i;

	punyopt(argc, argv, myopt, "k:w:r:o:");
	threads = Option.numthreads;
	start   = Option.dir;
	width   = Myopt.width;
	depth   = Myopt.depth;
	from    = Myopt.from;
	to      = Myopt.to;

	init(start);

	for (i = 0; i < Option.iterations; i++) {
		srandom(137);

		startTimer();
		start_threads(threads, width, depth, from, to);
		stopTimer();

		pr_inst( &Inst);

		prTimer();

		printf("\n");

		clear_inst();
		Done = FALSE;
	}
	return 0;
}
