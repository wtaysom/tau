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

static u64 A[NUM_SYS_CALLS];
static u64 B[NUM_SYS_CALLS];

u64 *Old = A;
u64 *New = B;
int Delta[NUM_SYS_CALLS];
void *Rank_pidcall[MAX_PIDCALLS];
int Num_rank;
TopPidCall_s Top_pid_call[MAX_TOP];

TickCounter_s Total_delta;

static int compare_pidcall(const void *a, const void *b)
{
	const PidCall_s *p = *(const PidCall_s **)a;
	const PidCall_s *q = *(const PidCall_s **)b;

	if (p->save.count > q->save.count) return -1;
	if (p->save.count == q->save.count) return 0;
	return 1;
}

static void reduce_pidcall(void)
{
	static int interval = 0;
	PidCall_s *pc;
	int i;
	int j;
	int k;

	pthread_mutex_lock(&Count_lock);
	for (pc = Pid_call, k = 0; pc < &Pid_call[MAX_PIDCALLS]; pc++) {
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

	++interval;
	if (k > MAX_TOP) k = MAX_TOP;
	for (i = 0; i < k; i++) {
		pc = Rank_pidcall[i];
		if (pc->save.count < Top_pid_call[MAX_TOP - 1].count) break;
		for (j = 0; j < MAX_TOP; j++) {
			TopPidCall_s *tc = &Top_pid_call[j];
			if (pc->save.count >= tc->count) {
				memmove(&tc[1], tc, (MAX_TOP - j - 1));
				tc->pidcall  = pc->pidcall;
				tc->count    = pc->save.count;
				tc->interval = interval;
				tc->time     = pc->save.time;
				if (pc->name) {
					strncpy(tc->name, pc->name, MAX_THREAD_NAME);
					tc->name[MAX_THREAD_NAME - 1] = '\0';
				} else {
					tc->name[0] = '\0';
				}
				break;
			}
		}
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
	zero(Top_pid_call);
}

void *reduce(void *arg)
{
	ignore_pid(gettid());
	init_display();

	for (;;) {
		if (Halt) return NULL;
		delta();
		reduce_pidcall();
		Display();
		nanosleep(&Sleep, NULL);
	}
}
