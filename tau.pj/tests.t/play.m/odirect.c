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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <puny.h>
#include <timer.h>

typedef struct Buf_s {
	unint	size;
	u8	*data;
} Buf_s;

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

void writer (void)
{
	struct timespec sleep = { Option.sleep_secs, 0 * ONE_MILLION };
	int	fd;
	int	rc;
	Buf_s	buf;
	unint	loops;
	unint	i;

	buf = prepare_buf();
	for (loops = 0; loops < Option.loops; loops++) {
		fd = open(Option.file, O_WRONLY | O_CREAT | O_TRUNC |O_DIRECT, 0700);
		if (fd == -1) fatal("open %s:", Option.file);
		unlink(Option.file);
		nanosleep(&sleep, NULL);
		u64 start = nsecs();
		for (i = 0; i < Option.iterations; i++) {
			rc = pwrite(fd, buf.data, buf.size, 0);
			if (rc == -1) fatal("unexpected write error %s:", Option.file);
		}
		u64 finish = nsecs();
		close(fd);
		pr_time(finish - start);
	}
	free(buf.data);
}

void usage (void)
{
	pr_usage("-f<file> -i<iterations> -l<loops> -s<secs to sleep>"
		" -z<buf size in pages\n"
		"\tf - file to write [%s]\n"
		"\ti - iterations [%lld]\n"
		"\tl - loops [%lld]\n"
		"\ts - seconds to sleep between loops [%lld]\n"
		"\tz - buf size in pages [%lld]",
		Option.file, Option.iterations, Option.loops, Option.sleep_secs,
		Option.file_size);
}

int main (int argc, char *argv[])
{
	Option.iterations = 100;
	Option.loops = 10;
	Option.file_size = 1000;
	punyopt(argc, argv, NULL, NULL);	
	writer();
	return 0;
}
