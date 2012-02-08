/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _KTOP_H_
#define _KTOP_H_ 1

#include <sys/types.h>
#include <style.h>
#include "syscall.h"

enum {	MAX_PID = 1 << 15,
	MAX_PIDCALLS = 1 << 10,
	MAX_NAME = 1 << 12,
	SYSCALL_SHIFT = 9,
	SYSCALL_MASK  = (1 << SYSCALL_SHIFT) - 1,
	NUM_ARGS = 6,
	MAX_TOP = 10,
	MAX_THREAD_NAME = 40 };

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

typedef void (*display_fn)(void);

typedef struct Display_s {
	display_fn	normal;
	display_fn	help;
} Display_s;

typedef struct Pidcall_s Pidcall_s;
struct Pidcall_s {
	Pidcall_s *next;	/* hash table chain */
	unint clock;		/* clock alg throws out old results */
	u32 pidcall;		/* combination of pid and system call */
	u32 count;		/* count of calls in last interval */
	u32 start_interval;	/* interval when created */
	struct {
		u64 start;	/* start time of last call */
		u64 total;	/* total time in this interval */
		u64 max;	/* max time in this interval */
	} time;
	struct {
		u32 count;	/* snapshot of count */
		u64 total_time;	/* snapshot of total time */
		u64 max_time;	/* snapshot of max time */
	} snap;
	struct {
		u32 max_count;	/* max count across intervals */
		u64 total_count;/* total count across intervals */
		u64 total_time;	/* total time used by this pidcall */
		u64 max_time;	/* Max time by any pidcall */
	} summary;
	char *name;
};

typedef struct TopPidcall_s {
	u32 pidcall;
	u32 count;
	u32 tick;
	u64 time;
	char name[MAX_THREAD_NAME];
} TopPidcall_s;

extern bool Dump;	/* Dump of ftrace logs - don't start display */
extern bool Trace_exit;	/* Trace sys_exit events */
extern bool Trace_self;	/* Trace myself and ignore others */
extern bool Pause;	/* Pause display */
extern bool Help;	/* Display help screen */
extern char *Log_file;	/* If set, use this file for logging */

extern Display_s Display;

extern u64 *Syscall_count;
extern int Pid[MAX_PID];
extern Pidcall_s Pidcall[MAX_PIDCALLS];
extern u64 Pidcall_record;
extern u64 Pidcall_iterations;
extern u64 Pidcall_tick;

extern u32 Current_interval;

extern TopPidcall_s Top_pidcall[MAX_TOP];

extern u64 No_enter;
extern u64 Found;
extern u64 Out_of_order;
extern u64 No_start;
extern u64 Bad_type;

extern u64 Ignored_pid_count;
extern u64 Slept;
extern bool Halt;

extern Display_s Internal_display;
extern Display_s Kernel_display;
extern Display_s Plot_display;
extern Display_s File_system_display;
extern Display_s Summary_display;

void cleanup(int sig);

void cleanup_collector(void);
void start_collector(void);

void *reduce(void *arg);
void decrease_reduce_interval(void);
void increase_reduce_interval(void);
void reset_reduce(void);

void cleanup_display(void);
void clear_display(void);

pid_t gettid(void);
void ignore_pid(int pid);
bool do_ignore_pid(int pid);

void graph(void);

void open_log(void);
void close_log(void);
void log_pidcalls(void);

/* Rounded integer divide - x/y -> (x + y/2) / y */
#define ROUNDED_DIVIDE(x, y)	((y) ? (((x) + (y) / 2) / (y)) : 0)

#endif
