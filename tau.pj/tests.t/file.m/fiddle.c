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


enum { MAX_PATH = 1024, MAX_NAME = 32, BLK_SIZE = 4096 };

struct {
	u64	num_opens;
	u64	num_dirs;
	u64	num_files;
	u64	num_deleted;
	u64	num_read;
	u64	num_written;
} Inst;

typedef struct arg_s {	/* per task arguments */
	unsigned	seed;
	unint		id;
	unint		num_files;
	unint		size;
	unint		fiddle;
} arg_s;

static void randomize (char *buf, unsigned size, arg_s *a)
{
	static char	rnd_char[] = "abcdefghijklmnopqrstuvwxyz\n";
	unsigned	i;

	for (i = 0; i < size; i++) {
		*buf++ = rnd_char[urand_r(sizeof(rnd_char), &a->seed)];
	}
}

void pr_inst (void)
{
	printf("opens   = %10llu\n", Inst.num_opens);
	printf("created = %10llu\n", Inst.num_dirs+Inst.num_files);
	printf("dirs    = %10llu\n", Inst.num_dirs);
	printf("files   = %10llu\n", Inst.num_files);
	printf("deleted = %10llu\n", Inst.num_deleted);
	printf("read    = %10llu\n", Inst.num_read);
	printf("written = %10llu\n", Inst.num_written);
}

void clear_inst (void)
{
	zero(Inst);
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

static int write_block (int fd, int block, arg_s *arg)
{
	char	buf[BLK_SIZE];
	u64	offset;
	int	rc;

	randomize(buf, sizeof(buf), arg);
	offset = block * sizeof(buf);
	lseek(fd, offset, 0);
	rc = write(fd, buf, sizeof(buf));
	if (rc == -1) {
		eprintf("write_block:");
		return -1;
	}
	return 0;
}

void fill (int fd, unsigned size, arg_s *arg)
{
	char		buf[BLK_SIZE];
	unsigned long	i;

	randomize(buf, sizeof(buf), arg);
	for (i = 0; i < size; i++) {
		if (write(fd, buf, sizeof(buf)) < 0) {
			fatal("write:");
		}
	}
	Inst.num_written += size;
}

int cr_file (char *name, unsigned size, arg_s *arg)
{
	int	fd;

	fd = creat(name, 0666);
	if (fd == -1) {
		eprintf("cr_file \"%s\" :", name);
		return -1;
	}
	++Inst.num_files;
	fill(fd, size, arg);
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

PRs(name);
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

void fiddle (char *path, arg_s *a)
{
	int	fd;
	int	rc;
	int	i;

	fd = open_file(path);
	if (fd == -1) return;

	for (i = a->size; --i, i > 0; ) {
		rc = write_block(fd, i, a);
		if (rc) return;
	}
	close(fd);
}

void mk_path (char *path, char *dir, int i)
{
	sprintf(path, "%s/file_%d", dir, i);
}

void *test (void *arg)
{
	char	path[MAX_PATH];
	char	dir[MAX_NAME];
	arg_s	*a = arg;
	int	i;
	int	rc;

	// make and cd to directory for thread to play in
	snprintf(dir, MAX_NAME-1, "fiddle_dir_%ld", a->id);
	cr_dir(dir);

	// create the number of files with the specific amount of data
	for (i = 0; i < a->num_files; i++) {
		mk_path(path, dir, i);
		rc = cr_file(path, a->size, a);
		if (rc) break;
	}
	a->num_files = i;

	// randomly open files and fiddle with their data
	for (i = 0; i < a->fiddle; i++) {
		mk_path(path, dir, urand_r(a->num_files, &a->seed));
		fiddle(path, a);
	}

	// delete the files
	for (i = 0; i < a->num_files; i++) {
		mk_path(path, dir, i);
		del_file(path);
	}

	// remove directory
	del_dir(dir);
	return NULL;
}

void start_threads (
	unsigned	threads,
	unsigned	num_files,
	unsigned	size,
	unsigned	fiddle)
{
	pthread_t	*thread;
	unsigned	i;
	int		rc;
	arg_s		*arg;
	arg_s		*a;

	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		a->seed      = random();
		a->id        = i;
		a->num_files = num_files;
		a->size      = size;
		a->fiddle    = fiddle;

		rc = pthread_create( &thread[i], NULL, test, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

u64	Numfiles = 2;
u64	Fiddle = 2;

bool myopt (int c)
{
	switch (c) {
	case 'k':
		Numfiles = strtoll(optarg, NULL, 0);
		break;
	case 'q':
		Fiddle = strtoll(optarg, NULL, 0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void usage (void)
{
	pr_usage("-t<threads> -k<num_files> -z<size>"
		" -q<fiddle> -i<iterations>");
}

int main (int argc, char *argv[])
{
	unsigned	threads;
	unsigned	num_files;
	unsigned	size;
	unsigned	fiddle;
	unsigned	iterations;
	unsigned	i;

	punyopt(argc, argv, myopt, "k:q:");
	threads    = Option.numthreads;
	num_files  = Numfiles;
	size       = Option.file_size;
	iterations = Option.iterations;
	fiddle     = Fiddle;
	if (!threads || !num_files || !size || !fiddle || !iterations) {
		usage();
	}
	for (i = 0; i < iterations; i++) {
		srandom(137);

		startTimer();
		start_threads(threads, num_files, size, fiddle);
		stopTimer();

		pr_inst();

		prTimer();

		printf("\n");

		clear_inst();
	}
	return 0;
}
