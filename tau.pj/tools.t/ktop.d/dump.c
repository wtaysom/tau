/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <unistd.h>

#include "collector.h"
#include "ktop.h"
#include "syscall.h"

void pr_buf(int cpu, int sz, u8 buf[sz])
{
	int i;
	int j;

	printf("%d. trace=%d bytes\n", cpu, sz);
	for (i = 0; i < sz; i++) {
		for (j = 0; j < 32; j++, i++) {
			if (i == sz) goto done;
			printf(" %2x", buf[i]);
		}
		printf("\n");
	}
done:
	printf("\n");
}

static void pr_event(event_s *event)
{
	printf(" type=%2u flags=%2x cnt=%2d pid=%5d lock=%2d",
		event->type, event->flags, event->preempt_count, event->pid,
		event->lock_depth);
}

static void pr_sys_enter(void *event)
{
	sys_enter_s *sy = event;

	printf(" %-20s", Syscall[sy->id]);
	printf("\n");
}

static void pr_sys_exit(void *event)
{
	sys_exit_s *sy = event;

	printf(" %-20s ret=%ld\n", Syscall[sy->id], sy->ret);
}

void pr_ring_header(ring_header_s *rh)
{
	printf("%lld %lld %ld\n",
		rh->time_stamp / ONE_BILLION, rh->time_stamp % ONE_BILLION,
		rh->commit);
}

void dump_event(void *buf)
{
	event_s *event = buf;

	pr_event(event);
	if (event->type == Sys_exit) {
		pr_sys_exit(event);
	} else if (event->type == Sys_enter) {
		pr_sys_enter(event);
	} else {
		printf(" no processing\n");
	}
}

static void dump_raw(int cpu, int sz, u8 buf[sz])
{
	parse_buf(buf);	// Need to do something with sz
}

void *dump_collector(void *args)
{
	Collector_args_s *a = args;
	u8 buf[BUF_SIZE];
	int cpu = a->cpu_id;
	int trace_pipe;
	int rc;
	int i;

	ignore_pid(gettid());
	trace_pipe = open_raw(cpu);
	for (i = 0;; i++) {
		rc = read(trace_pipe, buf, sizeof(buf));
		printf("i=%d rc=%d\n", i, rc);
		if (rc == -1) {
			close(trace_pipe);
			cleanup(0);
		}
		dump_raw(cpu, rc, buf);
		if (rc < SMALL_READ) {
			sleep(1);	// Wait for input to accumulate
		}
	}
	return NULL;
}
