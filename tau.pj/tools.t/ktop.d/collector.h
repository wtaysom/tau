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
 * Right now, the /usr/include/linux header files do not match
 * the version of the kernel, so I'm commenting out this code
 * until we decide the best course to solve this problem.
 */

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

//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#if 0

	#define EVENT_PATH	"events/syscalls/%s/enable"

	enum {	SYS_EXIT  = 21,
		SYS_ENTER = 22};
#else

	#define EVENT_PATH	"events/raw_syscalls/%s/enable"

	enum {	SYS_EXIT  = 15,
		SYS_ENTER = 16};
#endif

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

#endif
