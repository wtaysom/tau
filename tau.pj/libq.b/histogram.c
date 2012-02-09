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
#include <string.h>
#include <bit.h>

#include "histogram.h"

void incHistogram (Histogram_s *histogram)
{
	unint	level = ++histogram->currentCount;

	++histogram->totalCount;
	++histogram->bucket[flsl(level)];

	if (level > histogram->highWaterMark)
	{
		histogram->highWaterMark = level;
	}
}

void eventHistogram (Histogram_s *histogram, unint event)
{
	histogram->currentCount = event;
	histogram->eventSum += event;
	++histogram->totalCount;
	++histogram->bucket[flsl(event)];

	if (event > histogram->highWaterMark)
	{
		histogram->highWaterMark = event;
	}
}

void prHistogram (Histogram_s *histogram)
{
	unint	i;

	printf("High water mark %lu", histogram->highWaterMark);
	printf("\tTotal count %lu\n", histogram->totalCount);
	if (histogram->eventSum != 0)
	{
		printf("Sum of events %llu", histogram->eventSum);
		if (histogram->totalCount != 0)
		{
			u64	average = histogram->eventSum / histogram->totalCount;

			printf("\tAvg event %llu\n", average);
		}
		else
		{
			printf("\n");
		}
	}
	for (i = 0; i < NUM_BUCKETS; ++i)
	{
		printf("%2ld. %10lu%c", i, histogram->bucket[i],
			(i%4 == 3) ? '\n' : ' ');
	}
}


typedef struct RegisteredHistogram_s
{
	char	*name;
	bool	*isEnabled;
	Histogram_s	*histogram;
} RegisteredHistogram_s;

enum { MAX_HISTOGRAMS = 20 };

RegisteredHistogram_s	RegisteredHistograms[MAX_HISTOGRAMS];
unint	NumHistograms = 0;

RegisteredHistogram_s *lookupHistogram (char *name)
{
	unint	i;

	for (i = 0; i < NumHistograms; ++i)
	{
		if (strcmp(name, RegisteredHistograms[i].name) == 0)
		{
			return &RegisteredHistograms[i];
		}
	}
	return NULL;
}

int registerHistogram (
	char	*name,
	bool	*isEnabled,
	Histogram_s	*histogram)
{
	if (NumHistograms == MAX_HISTOGRAMS)
	{
		printf("No room for histogram %s\n", name);
		return -1;
	}
	if (lookupHistogram(name) != NULL)
	{
		printf("Already registered %s\n", name);
		return -1;
	}
	RegisteredHistograms[NumHistograms].name      = name;
	RegisteredHistograms[NumHistograms].isEnabled = isEnabled;
	RegisteredHistograms[NumHistograms].histogram = histogram;
	++NumHistograms;
	return 0;
}

RegisteredHistogram_s *findHistogram (char *name)
{
	unint	i;

	for (i = 0; i < NumHistograms; ++i)
	{
		if (strcmp(name, RegisteredHistograms[i].name) == 0)
		{
			return &RegisteredHistograms[i];
		}
	}
	printf("Histogram :%s: not found\n", name);
	return NULL;
}

struct PCLSwitchDef_s *switchDef;
int doListHistograms (
	struct PCLSwitchDef_s *switchDef,
	unint options,
	void *userParm)
{
	unint	i;

	for (i = 0; i < NumHistograms; ++i)
	{
		printf("%s\n", RegisteredHistograms[i].name);
	}
	return 0;
}
