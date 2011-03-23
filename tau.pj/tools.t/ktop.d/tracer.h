/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _TRACER_H
#define _TRACER_H 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _SYSCALL_H_
#include "syscall.h"
#endif

enum {	MAX_PID = 40000,
	MAX_PIDCALLS = 1 << 13,
	SYSCALL_SHIFT = 9,
	SYSCALL_MASK  = (1 << SYSCALL_SHIFT) - 1 };

CHECK_CONST((1 << SYSCALL_SHIFT) >= NUM_SYS_CALLS);

static inline u32 mkpidcall(int pid, int syscall)
{
	return pid << SYSCALL_SHIFT | syscall;
}

static inline u32 get_pid(int pidcall)
{
	return pidcall >> SYSCALL_SHIFT;
}

static inline u32 get_call(u32 pidcall)
{
	return pidcall & SYSCALL_MASK;
}

typedef struct Pidcall_s {
	u32 pidcall;
	u32 count;
} Pidcall_s;

extern u64 Syscall_count[NUM_SYS_CALLS];
extern int Pid[MAX_PID];
extern Pidcall_s Pidcall[MAX_PIDCALLS];
extern Pidcall_s *Pidnext;

extern int Command_tid;
extern int Collector_tid;
extern int Display_tid;
extern u64 MyPidCount;
extern u64 Slept;
extern int Command;

void cleanup(int sig);

void *collector(void *arg);
void cleanup_collector(void);

void *display(void *arg);
void cleanup_display(void);
void clear_display(void);

pid_t gettid(void);

#endif
