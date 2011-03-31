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

typedef unsigned long long	u64;

FILE *Results;

static inline u64 nsecs (void)
{
	struct timespec t;

	clock_gettime(CLOCK_REALTIME, &t);
	return (u64)t.tv_sec * 1000000000ULL + t.tv_nsec;
}

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

static void usage(char *progname)
{
	fprintf(stderr, "usage: %s [-?]"
		" [<file size> [<iterations> [<file name> [<results>]]]]\n"
		"\tdefaults: 1 1000000 .scratch stdout\n"
		"\t? - usage\n",
		progname);
	exit(2);
}

int main(int argc, char *argv[])
{
	char *name = ".scratch";
	char *results = "stdout";
	int fd;
	void *buf;
	size_t size = 1;
	u64 start;
	u64 finish;
	u64 total;
	int iterations = 1000000;
	int c;

	Results = stdout;
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
	if (argc > 4) {
		results = argv[4];
		Results = fopen(results, "a");
		if (!Results) return 3;
	}
	time_t timestamp;
	timestamp = time(NULL);
	fprintf(Results, "\n%s", ctime(&timestamp));
	fprintf(Results, "i/o size=%zd iterations=%d scratch=%s results=%s\n",
		size, iterations, name, results);

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
