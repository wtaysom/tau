/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <eprintf.h>
#include <puny.h>
#include <timer.h>

struct {
	bool	read;
	bool	write;
} Local_option = { TRUE, TRUE };

FILE *Results;

static void inplace_write(int fd, void *buf, size_t n, int iterations)
{
	while (iterations--) {
		ssize_t rc = pwrite(fd, buf, n, 0);
		if (rc != n) {
			perror("pwrite");
			exit(2);
		}
	}
}

static void inplace_read(int fd, void *buf, size_t n, int iterations)
{
	while (iterations--) {
		ssize_t rc = pread(fd, buf, n, 0);
		if (rc != n) {
			perror("pread");
			exit(2);
		}
	}
}

static int init(char *name, size_t size)
{
	size_t i;
	int fd;

	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror(name);
		exit(2);
	}
	for (i = 0; i < size; i++) {
		ssize_t rc = write(fd, "a", 1);
		if (rc != 1) {
			perror("write");
			exit(2);
		}
	}
	return fd;
}

static void cleanup(char *name, int fd)
{
	close(fd);
	unlink(name);
}

static void report(char *label, u64 total, size_t n, int iterations)
{
	double avg = (double)total / (double)iterations;
	fprintf(Results, "%s: total = %lld nanoseconds for %d iterations"
		" avg = %g nsecs  %g nsecs/byte\n",
		label, total, iterations, avg, avg / n);
}

void usage(void)
{
	pr_usage("[-RW] [-f<file>] [-z<file size>]"
	         " [-i<iterations>] [-r<results_file>]\n"
	         "\tR - turn off read test\n"
	         "\tW - turn off write test\n"
	         "\tf - file to use for test [%s]\n"
	         "\tz - size of file [%d]\n"
	         "\ti - number of iterations [%d]\n"
	         "\tr - results files [%s]\n",
	         Option.file,
	         Option.file_size,
	         Option.iterations,
	         Option.results);
}

bool localoptions (int c)
{
	switch (c) {
	case 'R':
		Local_option.read = FALSE;
		break;
	case 'W':
		Local_option.write = FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int main(int argc, char *argv[])
{
	char *name;
	char *results;
	int fd;
	void *buf;
	size_t size;
	u64 start;
	u64 finish;
	u64 total;
	int iterations;

	punyopt(argc, argv, localoptions, "RW");
	name = Option.file;
	size = Option.file_size;
	iterations = Option.iterations;
	if (Option.results) {
		results = Option.results;
		Results = fopen(Option.results, "a");
		if (!Results) return 3;
	} else {
		results = "stdout";
		Results = stdout;
	}
	time_t timestamp;
	timestamp = time(NULL);
	fprintf(Results, "\n%s", ctime(&timestamp));
	fprintf(Results, "i/o size=%zd iterations=%d scratch=%s results=%s\n",
		size, iterations, name, results);

	fd = init(name, size);
	buf = malloc(size);

	if (Local_option.write) {
		start = nsecs();
		inplace_write(fd, buf, size, iterations);
		finish = nsecs();
		total = finish - start;
		report("pwrite", total, size, iterations);
	}
	if (Local_option.read) {
		start = nsecs();
		inplace_read(fd, buf, size, iterations);
		finish = nsecs();
		total = finish - start;
		report("pread ", total, size, iterations);
	}
	cleanup(name, fd);
	return 0;
}
