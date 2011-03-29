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
#include <unistd.h>

#include "timer.h"

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

static void report(char *label, tick_t total, size_t n, int iterations)
{
	double avg = (double)total / (double)iterations;
	printf("%s: total = %lld nanoseconds for %d iterations"
		" avg = %g nsecs  %g nsecs/byte\n",
		label, total, iterations, avg, avg / n);
}

static void usage(char *progname)
{
	fprintf(stderr, "usage: %s [-?]"
		" [<file size> [<iterations> [<file name>]]]\n"
		"\tdefaults: 1 1000000 .scratch\n"
		"\t? - usage\n",
		progname);
	exit(2);
}

int main(int argc, char *argv[])
{
	char *name = ".scratch";
	int fd;
	void *buf;
	size_t size = 1;
	tick_t start;
	tick_t finish;
	tick_t total;
	int iterations = 1000000;
	int c;

	while ((c = getopt(argc, argv, "?")) != -1) {
		switch (c) {
		case '?':
			usage(argv[0]);
			break;
		default:
			fprintf(stderr, "unknown option %c\n", c);
			usage(argv[0]);
			break;
		}
	}
	if (argc > 1) {
		size = strtol(argv[1], NULL, 0);
	}
	if (argc > 2) {
		iterations = strtol(argv[2], NULL, 0);
	}
	if (argc > 3) {
		name = argv[3];
	}
	printf("i/o size = %zd iterations = %d name = %s\n",
		size, iterations, name);

	fd = init(name, size);
	buf = malloc(size);

	start = nsecs();
	inplace_write(fd, buf, size, iterations);
	finish = nsecs();
	total = finish - start;
	report("pwrite", total, size, iterations);

	start = nsecs();
	inplace_read(fd, buf, size, iterations);
	finish = nsecs();
	total = finish - start;
	report("pread ", total, size, iterations);

	cleanup(name, fd);
	return 0;
}
