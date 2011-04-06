/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#ifndef _TICKCOUNTER_H
#define _TICKCOUNTER_H 1

enum {	WHEEL_SIZE = 60 };

typedef struct Avg_s {
	u32	avg;
	u32	peak;
} Avg_s;

typedef struct TickCounter_s {
	u32	tick_size;	/* Must be factor of WHEEL_SIZE */
	u32	ticks_per_wheel;
	u32	hour_wrapped;
	u8	itick;
	u8	iminute;
	u8	ihour;
	u32	tick[WHEEL_SIZE];
	Avg_s	minute[WHEEL_SIZE];
	Avg_s	hour[WHEEL_SIZE];
} TickCounter_s;

void tick(TickCounter_s *counter, u32 n);
bool init_counter(TickCounter_s *counter, u32 tick);

#endif
