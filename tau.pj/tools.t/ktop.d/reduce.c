/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "ktop.h"
#include "reduce.h"
#include "syscall.h"
#include "tickcounter.h"

extern pthread_mutex_t Count_lock;

struct timespec Sleep = { 1, 0 };

/*
 * A and B are two arrays of counters for sys_call events
 * that are swapped between old and new.
 */
static u64 A[NUM_SYS_CALLS];
static u64 B[NUM_SYS_CALLS];
u64 *Old = A;
u64 *New = B;

/* Difference between New and Old */
int Delta[NUM_SYS_CALLS];

/* Descending sorted array of counts for pid/system_calls */
void *Rank_pidcall[MAX_PIDCALLS];

/* Current number pid/sys_calls in Rank_pidcall array */
int Num_rank;

/* Top count for pid/sys_calls since last clear */
TopPidcall_s Top_pidcall[MAX_TOP];

/* Change in total count of sys_calls events */
TickCounter_s Total_delta;

/* Number of times reduce/display has been called. */
static int Num_ticks = 0;

static int compare_pidcall(const void *a, const void *b)
{
	const Pidcall_s *p = *(const Pidcall_s **)a;
	const Pidcall_s *q = *(const Pidcall_s **)b;

	if (p->save.count > q->save.count) return -1;
	if (p->save.count == q->save.count) return 0;
	return 1;
}

static void fill_top_pidcall(TopPidcall_s *tc, Pidcall_s *pc)
{
	tc->pidcall = pc->pidcall;
	tc->count   = pc->save.count;
	tc->tick    = Num_ticks;
	tc->time    = pc->save.time;
	if (pc->name) {
		strncpy(tc->name, pc->name, MAX_THREAD_NAME);
		tc->name[MAX_THREAD_NAME - 1] = '\0';
	} else {
		tc->name[0] = '\0';
	}
}

static void replace_top_pidcall(TopPidcall_s *insert_here, Pidcall_s *pc)
{
	TopPidcall_s *tc;

	/* see if this pidcall is already in list */
	for (tc = Top_pidcall; tc < &Top_pidcall[MAX_TOP]; tc++) {
		if (pc->pidcall == tc->pidcall) {
			if (pc->save.count <= tc->count) return;
			assert(tc >= insert_here);
			memmove(&insert_here[1], insert_here,
				(tc - insert_here) * sizeof(*tc));
			fill_top_pidcall(insert_here, pc);
			return;
		}
	}
	memmove(&insert_here[1], insert_here,
		(tc - insert_here - 1) * sizeof(*tc));
	fill_top_pidcall(insert_here, pc);
}

static void top_pidcall(void)
{
	Pidcall_s *pc;
	int i;
	int j;
	int num_top;

	++Num_ticks;
	if (Num_rank < MAX_TOP) {
		num_top = Num_rank;
	} else {
		num_top = MAX_TOP;
	}
	for (i = 0; i < num_top; i++) {
		pc = Rank_pidcall[i];
		if (pc->save.count < Top_pidcall[MAX_TOP - 1].count) break;
		for (j = 0; j < MAX_TOP; j++) {
			TopPidcall_s *tc = &Top_pidcall[j];
			if (pc->save.count >= tc->count) {
				replace_top_pidcall(tc, pc);
				break;
			}
		}
	}
}

static void reduce_pidcall(void)
{
	Pidcall_s *pc;
	int k;

	pthread_mutex_lock(&Count_lock);
	for (pc = Pidcall, k = 0; pc < &Pidcall[MAX_PIDCALLS]; pc++) {
		if (pc->count) {
			Rank_pidcall[k++] = pc;
			pc->save.count = pc->count;
			pc->count = 0;
			pc->save.time = pc->time.total;
			pc->time.total = 0;
		}
	}
	pthread_mutex_unlock(&Count_lock);
	Num_rank = k;

	qsort(Rank_pidcall, k, sizeof(void *), compare_pidcall);

	top_pidcall();
}

static void delta(void)
{
	u64 *tmp;
	int sum;
	int i;

	tmp = Old;
	Old = New;
	New = tmp;

	if (0) {
		pthread_mutex_lock(&Count_lock);
		memmove(New, Syscall_count, sizeof(Syscall_count));
		memset(Syscall_count, 0, sizeof(Syscall_count));
		pthread_mutex_unlock(&Count_lock);
	} else {
		memmove(New, Syscall_count, sizeof(Syscall_count));
	}

	sum = 0;
	for (i = 0; i < NUM_SYS_CALLS; i++) {
		if (0) {
			Delta[i] = New[i];
		} else {
			Delta[i] = New[i] - Old[i];
			sum += Delta[i];
		}
	}
	tick(&Total_delta, sum);
}

void decrease_reduce_interval(void)
{
	if (Sleep.tv_sec == 1) {
		Sleep.tv_sec = 0;
		Sleep.tv_nsec = 500 * ONE_MILLION;
	} else if (Sleep. tv_sec > 1) {
		Sleep.tv_sec >>= 1;
	} else if (Sleep.tv_nsec > ONE_MILLION) {
		Sleep.tv_nsec >>= 1;
	}
}

void increase_reduce_interval(void)
{
	if (Sleep.tv_nsec >= 500 * ONE_MILLION) {
		Sleep.tv_sec = 1;
		Sleep.tv_nsec = 0;
	} else if (Sleep. tv_sec >= 1) {
		Sleep.tv_sec <<= 1;
	} else {
		Sleep.tv_nsec <<= 1;
	}
}

void reset_reduce(void)
{
	zero(A);
	zero(B);
	zero(Pid);
	zero(Syscall_count);
	Ignored_pid_count = 0;
	Slept = 0;
	zero(Top_pidcall);
}

void *reduce(void *arg)
{
	ignore_pid(gettid());
	init_display();

	for (;;) {
		if (Halt) return NULL;
		delta();
		reduce_pidcall();
		if (!Pause) Display();
		nanosleep(&Sleep, NULL);
	}
}
