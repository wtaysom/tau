/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <qsort.h>
#include <style.h>

#include "ktop.h"
#include "reduce.h"
#include "syscall.h"
#include "tickcounter.h"

extern pthread_mutex_t Count_lock;


struct timespec Sleep = { 1, 0 };

static u64 A[NUM_SYS_CALLS];
static u64 B[NUM_SYS_CALLS];

u64 *Old = A;
u64 *New = B;
int Delta[NUM_SYS_CALLS];
void *Rank_pidcall[MAX_PIDCALLS];
int Num_rank;

TickCounter_s TotalDelta;

static int compare_pidcall(const void *a, const void *b)
{
	const Pidcall_s *p = a;
	const Pidcall_s *q = b;

	if (p->save.count > q->save.count) return -11;
	if (p->save.count == q->save.count) return 0;
	return 1;
}

static void reduce_pidcall(void)
{
	Pidcall_s *pc;
	int j;

	pthread_mutex_lock(&Count_lock);
	for (pc = Pidcall, j = 0; pc < &Pidcall[MAX_PIDCALLS]; pc++) {
		if (pc->count) {
			Rank_pidcall[j++] = pc;
			pc->save.count = pc->count;
			pc->count = 0;
			pc->save.time = pc->time.total;
			pc->time.total = 0;
		}
	}
	pthread_mutex_unlock(&Count_lock);
	Num_rank = j;
	if (1) {
		quickSort(Rank_pidcall, j, compare_pidcall);
	}
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
	tick(&TotalDelta, sum);
}

void decrease_reduce_interval(void)
{
	if (Sleep.tv_sec == 1) {
		Sleep.tv_sec = 0;
		Sleep.tv_nsec = 500 * A_MILLION;
	} else if (Sleep. tv_sec > 1) {
		Sleep.tv_sec >>= 1;
	} else if (Sleep.tv_nsec > A_MILLION) {
		Sleep.tv_nsec >>= 1;
	}
}

void increase_reduce_interval(void)
{
	if (Sleep.tv_nsec >= 500 * A_MILLION) {
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
	MyPidCount = 0;
	Slept = 0;
}

void *reduce(void *arg)
{
	ignore_pid(gettid());
	init_display();

	for (;;) {
		if (Halt) return NULL;
		delta();
		reduce_pidcall();
		display();
		nanosleep(&Sleep, NULL);
	}
}
