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

/*
 * Convert UTC time to something human readable
 */

#include <stdio.h>

enum {
	SECS_PER_MIN  = 60,
	SECS_PER_HR   = SECS_PER_MIN * 60,
	SECS_PER_DAY  = SECS_PER_HR * 24,
	SECS_PER_YEAR = SECS_PER_DAY * 365,
	SECS_PER_LEAP = SECS_PER_YEAR + SECS_PER_DAY,
	SECS_PER_4YR  = (3 * SECS_PER_YEAR) + SECS_PER_LEAP
};

unsigned Time_zone_offset = 0;

char *date (unsigned long time)
{
	static char	*monthNames[] =
				{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	static unsigned	month[][12] =
				{{ 31,28,31,30,31,30,31,31,30,31,30,31 },
				 { 31,29,31,30,31,30,31,31,30,31,30,31 }};
	static char	d[45] = "The date";
	unsigned	sec, min, hr;
	unsigned	year, mon, day;
	unsigned	notleapyear, leapyear;

	time -= Time_zone_offset*SECS_PER_HR;
	for (year = 1970, notleapyear = 2; ;++year, --notleapyear) {
		if (notleapyear) {
			if (time < SECS_PER_YEAR) {
				break;
			}
			time -= SECS_PER_YEAR;
		} else {
			if (time < SECS_PER_LEAP) {
				break;
			}
			time -= SECS_PER_LEAP;
			notleapyear = 4;
		}
	}
	leapyear = !notleapyear;
	for (mon = 0; ;++mon) {
		unsigned	secspermon =
					month[leapyear][mon] * SECS_PER_DAY;

		if (time < secspermon) {
			break;
		}
		time -= secspermon;
	}
	day = time / SECS_PER_DAY + 1;
	time %= SECS_PER_DAY;
	hr = time / SECS_PER_HR;
	time %= SECS_PER_HR;
	min = time / SECS_PER_MIN;
	sec = time % SECS_PER_MIN;

	sprintf(d, "%.2d%s%.2d %.2d:%.2d:%.2d",
			year%100, /*(leapyear?"T":"F"),*/
			monthNames[mon], day, hr, min, sec);

	return d;
}

#if 0
#include <sys/types.h>
#include <sys/stat.h>

int main (int argc, char *argv[])
{
	struct stat	sb;
	int	rc;

	rc = stat("t", &sb);
	printf("%d=%s\n", sb.st_mtime, date(sb.st_mtime));

	return 0;
}
#endif
