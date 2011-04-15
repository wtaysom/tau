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
	int i;

	printf(" %-20s", Syscall[sy->id]);
	for (i = 0; i < 6; i++) {
		printf(" %ld", sy->args[i]);
	}
	printf("\n");
}

static void pr_sys_exit(void *event)
{
	sys_exit_s *sy = event;

	printf(" %-20s ret=%ld\n", Syscall[sy->id], sy->ret);
}

static void pr_ring_header(ring_header_s *rh)
{
	printf("%lld %lld %ld\n",
		rh->time_stamp / A_BILLION, rh->time_stamp % A_BILLION,
		rh->commit);
}

static void dump_event(void *buf)
{
	event_s *event = buf;

	pr_event(event);
	switch (event->type) {
	case 21:
		pr_sys_exit(event);
		break;
	case 22:
		pr_sys_enter(event);
		break;
	default:
		printf(" no processing\n");
		break;
	}
}

static void dump_buf(u8 *buf)
{
	ring_header_s *rh = (ring_header_s *)buf;
	ring_event_s *r;
	unint length;
	unint size;
	u64 time;
	u8 *end;

	pr_ring_header(rh);
	time = rh->time_stamp;
	buf += sizeof(*rh);
	end  = &buf[rh->commit];
	for (; buf < end; buf += size) {
		r = (ring_event_s *)buf;
		printf("type_len=%2u time=%9d", r->type_len, r->time_delta);
		if (r->type_len == 0) {
			length = r->array[0];
			size   = 4 + length * 4;
			time  += r->time_delta;
		} else if (r->type_len <= 28) {
			length = r->type_len;
			size   = 4 + length * 4;
			time  += r->time_delta;
			dump_event(buf+4);
		} else if (r->type_len == 29) {
			printf("\n");
			if (r->time_delta == 0) {
				return;
			} else {
				length = r->array[0];
				size = 4 + length * 4;
			}
		} else if (r->type_len == 30) {
			/* Extended time delta */
			printf("\n");
			size = 8;
			time += (((u64)r->array[0]) << 28) | r->time_delta;
		} else if (r->type_len == 31) {
			/* Sync time with external clock (NOT IMMPLEMENTED) */
			//tv_nsec = r->array[0];
			//tv_sec  = *(u64 *)&(r->array[1]);
		} else {
			printf(" Unknown event %d\n", r->type_len);
			/* Unknown - ignore */
			size = 4;
		}
	}
}

static void dump_raw(int cpu, int sz, u8 buf[sz])
{
	dump_buf(buf);	// Need to do something with sz
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
