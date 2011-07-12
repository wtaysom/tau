/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <string.h>

#include <style.h>

#include "tickcounter.h"

bool tick_is_valid(u32 tick)
{
	return (WHEEL_SIZE % tick) == 0;
}

bool init_counter(TickCounter_s *counter, u32 tick)
{
	if (!tick_is_valid(tick)) return FALSE;
	zero(*counter);
	counter->tick_size = tick;
	counter->ticks_per_wheel = WHEEL_SIZE / tick;
	return TRUE;
}

static u32 average(u32 x, u32 q)
{
	return (x + q / 2) / q;
}

/*
 * Even though a tick may be multiple seconds, results are
 * normalized to seconds;
 */
void tick(TickCounter_s *counter, u32 n)
{
	int x;
	int sum;
	int max;
	int i;

	counter->tick[counter->itick] = n;
	if (++counter->itick < WHEEL_SIZE) return;
	for (sum = 0, max = 0, i = 0; i < WHEEL_SIZE; i++) {
		x = counter->tick[i];
		sum += x;
		if (x > max) max = x;
	}
	counter->itick = 0;
	counter->minute[counter->iminute].avg = average(sum, WHEEL_SIZE);
	counter->minute[counter->iminute].peak = average(max, counter->tick_size);
	if (++counter->iminute < WHEEL_SIZE) return;
	for (sum = 0, max = 0, i = 0; i < WHEEL_SIZE; i++) {
		sum += counter->minute[i].avg;;
		if (counter->minute[i].peak > max) {
			max = counter->minute[i].peak;
		}
	}
	counter->iminute = 0;
	counter->hour[counter->ihour].avg = average(sum, WHEEL_SIZE);
	counter->hour[counter->ihour].peak = max;
	if (++counter->ihour < WHEEL_SIZE) return;
	counter->ihour = 0;
	++counter->hour_wrapped;
}

void dump_counter(TickCounter_s *counter)
{
	int i;

	printf("Ticks: %d\n", counter->tick_size);
	for (i = 0; i < counter->ticks_per_wheel; i++) {
		printf("%2i. %d %d\n",
			i, counter->tick[i],
			average(counter->tick[i], counter->tick_size));
	}
	printf("Minutes:\n");
	for (i = 0; i < WHEEL_SIZE; i++) {
		printf("%2i. %d %d\n",
			i, counter->minute[i].avg,
			counter->minute[i].peak);
	}
	printf("Hours:\n");
	for (i = 0; i < WHEEL_SIZE; i++) {
		printf("%2i. %d %d\n",
			i, counter->hour[i].avg,
			counter->hour[i].peak);
	}
}

#if 0
int main(int argc, char *argv[])
{
	TickCounter_s counter;
	int i;

	init_counter(&counter, 3);
	for (i = 0; i < 1000000; i++) {
		tick(&counter, urand(21));
	}
	dump_counter(&counter);
	return 0;
}
#endif
