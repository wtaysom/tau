/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#ifndef _COLLECTOR_H_
#define _COLLECTOR_H_

#include <linux/version.h>

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _KTOP_H_
#include <ktop.h>
#endif

enum {	BUF_SIZE = 1 << 12,
	SMALL_READ = BUF_SIZE >> 2 };

/*
 * The id and format for syscall events are defined in
 * /sys/kernel/debug/tracing/events/raw_syscalls/sys_enter/format
 * /sys/kernel/debug/tracing/events/raw_syscalls/sys_exit/format
 * and for older kernels
 * /sys/kernel/debug/tracing/events/syscalls/sys_enter/format
 * /sys/kernel/debug/tracing/events/syscalls/sys_exit/format
 *
 * A copy of ftrace.txt is in the ktop source directory but
 * you should consult the current kernel documentation in
 * Documentation/trace/ftrace.txt
 */

typedef struct ring_header_s {
	u64	time_stamp;
	unint	commit;
	u8	data[];
} ring_header_s;

typedef struct ring_event_s {
	u32	type_len   : 5,
		time_delta : 27;
	u32	array[];
} ring_event_s;

typedef struct event_s {
	u16	type;
	u8	flags;
	u8	preempt_count;
	s32	pid;
	s32	lock_depth;
} event_s;

typedef struct sys_enter_s {
	event_s	ev;
	snint	id;
	unint	arg[NUM_ARGS];
} sys_enter_s;

typedef struct sys_exit_s {
	event_s	ev;
	snint	id;
	snint	ret;
} sys_exit_s;

typedef struct Collector_args_s {
	int	cpu_id;
} Collector_args_s;

int open_raw(int cpu);
unint parse_buf(u8 *buf);
void *dump_collector(void *args);
void dump_event(void *buf);
void pr_ring_header(ring_header_s *rh);

extern int Sys_exit;
extern int Sys_enter;

#endif
