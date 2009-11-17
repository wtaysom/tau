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

#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Pre-define struct(s) so Linux compiler doesn't complain */
struct ScreenStruct;

#define NUM_BUCKETS	32

typedef struct Histogram_s
{
	unint	currentCount;
	unint	highWaterMark;
	u64	eventSum;
	unint	totalCount;
	unint	bucket[NUM_BUCKETS];
} Histogram_s;

void incHistogram(Histogram_s *histogram);
void eventHistogram(Histogram_s *histogram, unint event);
void prHistogram(Histogram_s *histogram);

int registerHistogram(
	char	*name,
	bool	*isEnabled,
	Histogram_s	*histogram);

#define HISTOGRAM ENABLE
#if HISTOGRAM IS_ENABLED

#define INC_HISTOGRAM(_hist)	(incHistogram( &(_hist)))
#define DEC_HISTOGRAM(_hist)	(--((_hist).currentCount))
#define ZERO_HISTOGRAM(_hist)	(bzero( &(_hist), sizeof(Histogram_s)))
#define EVENT_HISTOGRAM(_hist, _event)	(eventHistogram( &(_hist), (_event)))

#else

#define INC_HISTOGRAM(_hist)		((void) 0)
#define DEC_HISTOGRAM(_hist)		((void) 0)
#define ZERO_HISTOGRAM(_hist)		((void) 0)
#define EVENT_HISTOGRAM(_hist)		((void) 0)

#endif

#ifdef __cplusplus
}
#endif

#endif
