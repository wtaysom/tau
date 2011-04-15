/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#ifndef _COLLECTOR_H_
#define _COLLECTOR_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

enum {	BUF_SIZE = 1 << 12,
	SMALL_READ = BUF_SIZE >> 2 };

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
	unint	args[6];
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
void *dump_collector(void *args);

#endif
