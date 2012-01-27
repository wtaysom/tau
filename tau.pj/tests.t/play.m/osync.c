/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Periodically write a file with O_SYNC. Used to probe file system while
 * another test is running.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <puny.h>
#include <timer.h>

u8	Buf[1<<12] = { 72, 47 };

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

void writer (void)
{
	struct timespec sleep = { Option.sleep_secs, 0 * ONE_MILLION };
	int	fd;
	int	rc;

	fd = open(Option.file, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0700);
	if (fd == -1) fatal("open %s:", Option.file);
	unlink(Option.file);
	for (;;) {
		nanosleep(&sleep, NULL);
		u64 start = nsecs();
		rc = write(fd, Buf, sizeof(Buf));
		u64 finish = nsecs();
		if (rc == -1) fatal("unexpected write error %s:", Option.file);
		pr_time(finish - start);
	}
}

void usage (void)
{
	pr_usage("-f<file> -s<secs to sleep>\n"
		"\tf - file to write\n"
		"\ts - seconds to sleep between writes");
}

int main (int argc, char *argv[])
{
	punyopt(argc, argv, NULL, NULL);	
	writer();
	return 0;
}
