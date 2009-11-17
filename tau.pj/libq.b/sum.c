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
#include <math.h>

static int	N;
static int	MaxN;
static int	MinN;
static double	Sum;
static double	Sum2;
static double	Max;
static double	Min;

void init_sum (void)
{
	N    = 0;
	Sum  = 0.0;
	Sum2 = 0.0;
	Max  = 0.0;
	Min  = 0.0;
	MaxN = 0;
	MinN = 0;
}

void q_sum (long long x)
{
	++N;
	Sum  += x;
	Sum2 += (x * x);
	if (MaxN == 0) {
		Max = x;
		MaxN = N;
	} else if (x > Max) {
		Max = x;
		MaxN = N;
	}
	if (MinN == 0) {
		Min = x;
		MinN = N;
	} else if (x < Min) {
		Min = x;
		MinN = N;
	}
}

void f_sum (double x)
{
	++N;
	Sum  += x;
	Sum2 += (x * x);
	if (MaxN == 0) {
		Max = x;
		MaxN = N;
	} else if (x > Max) {
		Max = x;
		MaxN = N;
	}
	if (MinN == 0) {
		Min = x;
		MinN = N;
	} else if (x < Min) {
		Min = x;
		MinN = N;
	}
}

void pr_sum (char *msg)
{
	double	avg;
	double	stdv = 0.0;

	avg = Sum / N;
	if (N > 1) {
		stdv = sqrt((Sum2 - (Sum * Sum)/N) / (N - 1));
	}
	printf("%3d. %s avg=%5.3g stdv=%5.3g",
		N, msg, avg, stdv);
}

void pr_sum_min_max (char *msg)
{
	double	avg;
	double	stdv = 0.0;

	avg = Sum / N;
	if (N > 1) {
		stdv = sqrt((Sum2 - (Sum * Sum)/N) / (N - 1));
	}
	printf("%3d. %s avg=%5.3g stdv=%5.3g max=%d:%5.3g min=%d:%5.3g",
		N, msg, avg, stdv, MaxN, Max, MinN, Min);
}

#if 0
#include <stdio.h>

int main (int argc, char *argv)
{
	int	i;

	init_sum();
	for (i = 0; i < 10; ++i) {
		i_sum(i);
	}
	pr_sum("ans");
	return 0;
}
#endif
