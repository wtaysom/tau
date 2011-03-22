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

#ifndef _TIMER_H_
#define _TIMER_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef unsigned long long	tick_t;

enum { NUM_SLOTS = 16, NUM_WHEELS = 16 };

typedef struct slot_s {
	tick_t	s_delta;
	tick_t	s_avg;
} slot_s;

typedef struct wheel_s {
	unsigned	wh_next;
	slot_s		wh_slot[NUM_SLOTS];
} wheel_s;

typedef wheel_s	cascade_s[NUM_WHEELS];

void cascade (cascade_s wheel, tick_t delta);
void pr_cascade (const char *label, cascade_s wheel);

#ifdef __APPLE__

#include <sys/time.h>

static inline tick_t cputicks (void)
{
	struct timeval	time;
	tick_t		ticks;

	gettimeofday( &time, NULL);
	ticks = time.tv_sec * 1000000ULL + time.tv_usec;
	return ticks;
}

static inline tick_t nsecs (void)
{
	struct timeval	time;
	tick_t		ticks;

	gettimeofday( &time, NULL);
	ticks = time.tv_sec * 1000000ULL + time.tv_usec;
	return ticks * 1000;
}

#else

/*
 * i386 calling convention returns 64-bit value in edx:eax, while
 * x86_64 returns at rax. Also, the "A" constraint does not really
 * mean rdx:rax in x86_64, so we need specialized behaviour for each
 * architecture
 */
#ifdef __x86_64__
#define DECLARE_ARGS(val, low, high)	unsigned low, high
#define EAX_EDX_VAL(val, low, high)	((low) | ((u64)(high) << 32))
#define EAX_EDX_ARGS(val, low, high)	"a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high)	"=a" (low), "=d" (high)
#endif

#ifdef __i386__
#define DECLARE_ARGS(val, low, high)	unsigned long long val
#define EAX_EDX_VAL(val, low, high)	(val)
#define EAX_EDX_ARGS(val, low, high)	"A" (val)
#define EAX_EDX_RET(val, low, high)	"=A" (val)
#endif

// DON'T USE keeping here for future reference.
#if 0
static __always_inline unsigned long long cputicks (void)
{
	DECLARE_ARGS(val, low, high);

	// Really need barrier's but is very processor specific
	//rdtsc_barrier();
	asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));
	//rdtsc_barrier();

	return EAX_EDX_VAL(val, low, high);
}
#endif

#ifndef _TIME_H
#include <time.h>
#endif

/* Link with -lrt */

static inline u64 nsecs (void)
{
	struct timespec	t;

	clock_gettime(CLOCK_REALTIME, &t);

	return (u64)t.tv_sec * 1000000000ULL + t.tv_nsec;
}

#endif

#define TIME(_c, _x)	{	\
	u64	start;		\
	u64	end;		\
				\
	start = nsecs();	\
	_x;			\
	end = nsecs();	\
	cascade(_c, end-start);	\
}

#endif

#ifdef __cplusplus
}
#endif
