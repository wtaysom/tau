/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _KTOP_H_
#define _KTOP_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _SYSCALL_H_
#include "syscall.h"
#endif

enum {	MAX_PID = 1 << 15,
	MAX_PIDCALLS = 1 << 10,
	MAX_NAME = 1 << 12,
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

typedef struct Pidcall_s Pidcall_s;
struct Pidcall_s {
	Pidcall_s *next;
	u32 pidcall;
	u32 count;
	u64 start_time;
	u64 total_time;
	char *name;
};

extern bool Dump;	/* Dump of ftrace logs - don't start display */
extern bool Trace_exit;	/* Trace sys_exit events */
extern bool Trace_self;	/* Trace myself and ignore others */

extern u64 Syscall_count[NUM_SYS_CALLS];
extern int Pid[MAX_PID];
extern Pidcall_s Pidcall[MAX_PIDCALLS];
extern Pidcall_s *Pidnext;
extern u64 PidcallRecord;
extern u64 PidcallIterations;

extern u64 No_enter;
extern u64 Found;
extern u64 Out_of_order;
extern u64 No_start;

extern u64 MyPidCount;
extern u64 Slept;
extern int Command;

void cleanup(int sig);

void cleanup_collector(void);
void start_collector(void);

void *display(void *arg);
void cleanup_display(void);
void clear_display(void);
void decrease_display_interval(void);
void increase_display_interval(void);

pid_t gettid(void);
void ignore_pid(int pid);
bool do_ignore_pid(int pid);

void graph(void);

#endif
