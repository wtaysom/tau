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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <style.h>
#include <timer.h>

int main (int argc, char *argv[])
{
	unint	loops = 100000000;
	unint	i;
	u64	x, y;
	u64	start, finish;

	if (argc > 1) {
		loops = atoi(argv[1]);
	}
	x = y = 200000000000LLU;
	start = nsecs();
	for (i = loops; i; i--) {
		x += 2;
		y += x / i;
	}
	finish = nsecs();
	printf("loops=%ld x=%lld y=%lld nsecs/loop=%g\n",
		loops, x, y, ((double)(finish - start)) / loops);
	return 0;
}
