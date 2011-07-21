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
#include <sys/time.h>

#include <mystdlib.h>

static timeval_s	Start;
static timeval_s	End;

double current_time ()
{
	double		x;
	timeval_s	time;

	gettimeofday( &time, NULL);
	x = time.tv_sec + time.tv_usec / 1000000.0;

	return x;
}

void prTimer ()
{
	timeval_s	diff;
	double		x;

	diff.tv_sec = End.tv_sec - Start.tv_sec;
	diff.tv_usec = End.tv_usec - Start.tv_usec;
	if (End.tv_usec < Start.tv_usec) {
		diff.tv_usec += 1000000;
		--diff.tv_sec;
	}
	x = ((long long)diff.tv_sec * 1000000 + diff.tv_usec) / 1000000.0;
	printf("%5.3g ", x);
	f_sum(x);
	pr_sum("timer");
}

void startTimer ()
{
	gettimeofday( &Start, NULL);
}

void stopTimer()
{
	gettimeofday( &End, NULL);
}

void resetTimer ()
{
	init_sum();
}

#if 0

#include <stdlib.h>
#include <unistd.h>

int urand (int n)
{
	return random() % n;
}

int main (int argc, char *argv[])
{
	int		x;
	int		rc;
	unsigned	i;
	timeval_s	diff;

	for (i = 0; i < 300; ++i) {
		x = urand(1000000);
		startTimer();

		rc = usleep(x);

		stopTimer();
		if (rc == -1) perror("usleep");

		if (urand(100) < 3) resetTimer();
		printf("%d.%.6d == ", x/1000000, x%1000000);

		prTimer();
		printf("\n");
	}
	return 0;
}
#endif
