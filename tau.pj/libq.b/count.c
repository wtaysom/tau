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
#include <debug.h>


#if 0
#include "style.h"

#define CNT {								\
	static counter_s counter = { NULL, WHERE, __func__, 0 };	\
	count( &counter);						\
}

typedef struct counter_s counter_s;
struct counter_s {
	counter_s	*next;
	const char	*where;
	const char	*fn;
	uquad		count;
};
#endif

counter_s	*Counters = (counter_s *)&Counters;

void count (counter_s *counter)
{
	if (counter->count == 0) {
		counter->next = Counters;
		Counters = counter;
	}
	++counter->count;
}

void report (void)
{
	counter_s	*counter;

	for (	counter  = Counters;
		counter != (counter_s *)&Counters;
		counter  = counter->next)
	{
		printf("%s %s %lld\n",
			counter->where, counter->fn, counter->count);
	}
}

#if 0
int main (int argc, char *argv[])
{
	int	i;

	CNT;
	for (i = 0; i < 10; i++) {
		CNT;
	}
	report();
	return 0;
}
#endif
