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
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <style.h>
#include <timer.h>

int main (int argc, char *argv[])
{
	struct timeval tv;
	u64	start, finish;
	unint	i;
	unint	n = 10000;
	unint	sum = 0;

	if (argc > 1) {
		n = strtoll(argv[1], NULL, 0);
	}
	start = nsecs();
	sleep(1);
	finish = nsecs();
	printf("Timed second %llu - %llu = %llu nsecs\n",
		finish, start, finish - start);

	start = nsecs();
	for (i = 0; i < n; i++) {
		gettimeofday(&tv, NULL);
	}
	finish = nsecs();
	printf("gettimeofday overhead = %g nsecs\n", (double)(finish - start)/n);

	start = nsecs();
	for (i = 0; i < n; i++) {
		sum += i;
	}
	finish = nsecs();
	printf("gettimeofday loop overhead = %g nsecs sum = %lx\n",
		(double)(finish - start)/n, sum);

	return 0;
}


