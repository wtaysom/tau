/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _REDUCE_H_
#define _REDUCE_H_ 1

#ifndef _STYLE_H_
#inclucde <style.h>
#endif

#ifndef _SYSCALL_H_
#include <syscall.h>
#endif

#ifndef _KTOP_H_
#include <ktop.h>
#endif

#ifndef _TICKCOUNTER_H_
#include <tickcounter.h>
#endif

struct timespec Sleep;

extern u64 *Old;
extern u64 *New;
extern int Delta[NUM_SYS_CALLS];
extern void *Rank_pidcall[MAX_PIDCALLS];
extern int Num_rank;
extern TickCounter_s TotalDelta;

extern void init_display(void);
extern void display(void);

#endif
