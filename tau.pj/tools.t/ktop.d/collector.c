/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>

#include "collector.h"
#include "ktop.h"
#include "syscall.h"
#include "token.h"
#include "util.h"

#define _STR(x) #x
#define STR(x) _STR(x)
#define MAX_PATH 256

enum { PIDCALL_BUCKETS = 1543 };

pthread_mutex_t Count_lock = PTHREAD_MUTEX_INITIALIZER;
/*
 * Count_lock protects these global variables
 */
u64 *Syscall_count;
u64 Slept;
int Pid[MAX_PID];

Pidcall_s Pidcall[MAX_PIDCALLS];
Pidcall_s *Pidclock = Pidcall;
u64 Pidcall_iterations;
u64 Pidcall_record;
u64 Pidcall_tick;

u64 No_enter;
u64 Found;
u64 Out_of_order;
u64 No_start;
u64 Bad_type;

Pidcall_s *Pidcall_bucket[PIDCALL_BUCKETS];

int Sys_exit;
int Sys_enter;

static char Trace_path[MAX_PATH];
static char *Event_path;

static void find_trace_path(void)
{
	static const char tracing[] = "/tracing/";
	char type[100];
	FILE *fp;

	/*
	 * Have to find where "debugfs" is mounted.
	 */
	if (!(fp = fopen("/proc/mounts","r"))) {
		perror("/proc/mounts");
		return;
	}
	while (fscanf(fp, "%*s %"
			STR(MAX_PATH)
			"s %99s %*s %*d %*d\n",
			Trace_path, type) == 2) {
		if (strcmp(type, "debugfs") == 0) {
			strcat(Trace_path, tracing);
			fclose(fp);
			return;
		}
	}
	fclose(fp);
	fprintf(stderr, "debugfs not mounted");
}

static int get_event_id (char *sys_call)
{
	char file_name[MAX_PATH];
	char *t;

	snprintf(file_name, MAX_PATH,
		"%s/%s/%s/format",
		Trace_path, Event_path, sys_call);
	open_token(file_name, " \n\t");
	for (;;) {
		t = get_token();
		if (!t) break;
		if (strcmp(t, "ID:") == 0) {
			t = get_token();
			if (!t) break;
			close_token();
			return atoi(t);
		}
	}
	close_token();
	fatal("event id for %s not found in %s", sys_call, file_name);
	return 0;
}

static void init_tracing(void)
{
	find_trace_path();
	if (kernel_release() < release_to_int("2.6.38")) {
		Event_path = "events/syscalls";
	} else {
		Event_path = "events/raw_syscalls";
	}
}

static void update_event(const char *sys_call, const char *update)
{
	char file_name[MAX_PATH];
	FILE *file;
	int rc;

	snprintf(file_name, MAX_PATH,
		"%s/%s/%s/enable",
		Trace_path, Event_path, sys_call);
	file = fopen(file_name, "w");
	if (!file) {
		fatal("fopen %s:", file_name);
	}
	rc = fprintf(file, "%s\n", update);
	if (rc < 0) {
		fatal("fprintf %s:", file_name);
	}
	fclose(file);
}

static const char Enable[] = "1";
static const char Disable[] = "0";

static void enable_sys_enter(void)
{
	update_event("sys_enter", Enable);
	Sys_enter = get_event_id("sys_enter");
}

static void disable_sys_enter (void)
{
	update_event("sys_enter", Disable);
}

static void enable_sys_exit(void)
{
	update_event("sys_exit", Enable);
	Sys_exit = get_event_id("sys_exit");
}

static void disable_sys_exit (void)
{
	update_event("sys_exit", Disable);
}

int open_raw(int cpu)
{
	char name[MAX_NAME];
	int fd;

	snprintf(name, sizeof(name),
		"%s/per_cpu/cpu%d/trace_pipe_raw",
		Trace_path, cpu);
	fd = open(name, O_RDONLY);
	if (fd == -1) {
		fatal("open %s:", name);
	}
	return fd;
}

static Pidcall_s *hash_pidcall(u32 pidcall)
{
	return (Pidcall_s *)&Pidcall_bucket[pidcall % PIDCALL_BUCKETS];
}

static Pidcall_s *find_pidcall(u32 pidcall)
{
	Pidcall_s *pc = hash_pidcall(pidcall);

	++Pidcall_record;
	for (;;) {
		++Pidcall_iterations;
		pc = pc->next;
		if (!pc) return NULL;
		if (pc->pidcall == pidcall) {
			pc->clock = 1;
			return pc;
		}
	}
}

static void add_pidcall(Pidcall_s *pidcall)
{
	Pidcall_s *pc = hash_pidcall(pidcall->pidcall);

	pidcall->next = pc->next;
	pc->next = pidcall;
}

static void remove_pidcall(u32 pidcall)
{
	Pidcall_s *prev = hash_pidcall(pidcall);
	Pidcall_s *next;

	for (;;) {
		next = prev->next;
		if (!next) return;
		if (next->pidcall == pidcall) {
			prev->next = next->next;
			return;
		}
		prev = next;
	}
}

/* reallocate_pidcall finds a Pidcall slot using a clock
 * algorithm, throws away the data and gives it out
 * to be reused.
 */
static Pidcall_s *reallocate_pidcall(u32 pidcall)
{
	Pidcall_s *pc = Pidclock;

	while (pc->clock) {
		++Pidcall_tick;
		pc->clock = 0;
		if (++Pidclock == &Pidcall[MAX_PIDCALLS]) {
			Pidclock = Pidcall;
		}
		pc = Pidclock;
	}
	if (pc->pidcall) {
		remove_pidcall(pc->pidcall);
	}
	if (pc->name) {
		free(pc->name);
	}
	zero(*pc);
	pc->pidcall = pidcall;
	add_pidcall(pc);
	pc->start_interval = Current_interval;

	return pc;
}

static void parse_sys_enter(void *event, u64 time)
{
	sys_enter_s *sy  = event;
	int	pid      = sy->ev.pid;
	snint	call_num = sy->id;
	u32	pidcall  = mkpidcall(pid, call_num);
	Pidcall_s *pc;

	++Pid[pid];

	if (call_num >= Num_syscalls) {
		warn("syscall number out of range %ld\n", call_num);
		return;
	}
	++Syscall_count[call_num];

	pc = find_pidcall(pidcall);
	if (!pc) {
		pc = reallocate_pidcall(pidcall);
	}
	++pc->count;
	pc->time.start = time;
}

static void parse_sys_exit(void *event, u64 time)
{
	sys_exit_s *sy   = event;
	int	pid      = sy->ev.pid;
	snint	call_num = sy->id;
	u32	pidcall  = mkpidcall(pid, call_num);
	u64	time_used;
	Pidcall_s *pc;

	pc = find_pidcall(pidcall);
	if (!pc) {
		++No_enter;
		return;
	}
	if (pc->time.start) {
		if (time > pc->time.start) {
			++Found;
			time_used = time - pc->time.start;
			pc->time.total += time_used;
			if (time_used > pc->time.max) {
				pc->time.max = time_used;
			}
		} else {
			++Out_of_order;
		}
		pc->time.start = 0;
	} else {
		++No_start;
	}
}

static void parse_event(void *buf, u64 time)
{
	event_s *event = buf;

	/*
	 * Normally, ktop ignores any events relating to threads
	 * associated with itself. The -s option lets ktop
	 * monitor itself and ignore everything else.
	 */
	if (event->type == Sys_exit) {
		if (do_ignore_pid(event->pid) && !Trace_self) return;
		parse_sys_exit(event, time);
	} else if (event->type == Sys_enter) {
		if (do_ignore_pid(event->pid) && !Trace_self) return;
		parse_sys_enter(event, time);
	} else {
		//printf("%d no processing\n", event->type);
		++Bad_type;
	}
}

unint parse_buf(u8 *buf)
{
	ring_header_s *rh = (ring_header_s *)buf;
	ring_event_s *r;
	unint commit;
	unint length;
	unint size = 0;
	u64 time;
	u8 *end;

	if (Dump) pr_ring_header(rh);
	time = rh->time_stamp;
	commit = rh->commit;
	buf += sizeof(*rh);
	end = &buf[commit];
	pthread_mutex_lock(&Count_lock);
	for (; buf < end; buf += size) {
		r = (ring_event_s *)buf;
		if (r->type_len == 0) {
			/* Larger record where size is at beginning of record */
			length = r->array[0];
			size = 4 + length * 4;
			time += r->time_delta;
		} else if (r->type_len <= 28) {
			/* Data record */
			length = r->type_len;
			size = 4 + length * 4;
			time += r->time_delta;
			if (Dump) {
				dump_event(buf);
			} else {
				parse_event(buf+4, time);
			}
		} else if (r->type_len == 29) {
			/* Left over page padding or discarded event */
			if (r->time_delta == 0) {
				break;
			} else {
				length = r->array[0];
				size = 4 + length * 4;
			}
		} else if (r->type_len == 30) {
			/* Extended time delta */
			size = 8;
			time += (((u64)r->array[0]) << 28) | r->time_delta;
		} else if (r->type_len == 31) {
			/* Sync time with external clock (NOT IMMPLEMENTED) */
			size = 12;
			time = r->array[0];
			time += *(u64 *)&(r->array[1]) * ONE_BILLION;
		} else {
			warn(" Unknown event %d", r->type_len);
			/* Unknown - ignore */
			size = 4;
			break;
		}
		if (size > end - buf) {
			break;
		}
	}
	pthread_mutex_unlock(&Count_lock);
	return commit;
}

void *collector(void *args)
{
	Collector_args_s *a = args;
	/*
	 *  1 ms -> 7% overhead
	 * 10 ms -> 3% overhead
	 */
	struct timespec sleep = { 0, 10 * ONE_MILLION };
	u8 buf[BUF_SIZE];
	int cpu = a->cpu_id;
	int trace_pipe;
	int rc;
	int i;

	ignore_pid(gettid());
	trace_pipe = open_raw(cpu);
	for (i = 0;; i++) {
		rc = read(trace_pipe, buf, sizeof(buf));
		if (rc == -1) {
			close(trace_pipe);
			cleanup(0);
		}
		rc = parse_buf(buf);
		if (rc < SMALL_READ) {
			++Slept;
			nanosleep(&sleep, NULL);
		}
	}
	return NULL;
}

void cleanup_collector(void)
{
	disable_sys_enter();
	disable_sys_exit();
}

void start_collector(void)
{
	pthread_t collector_thread;
	int num_cpus;
	Collector_args_s *args;
	int i;
	int rc;

	Syscall_count = ezalloc(Num_syscalls * sizeof(*Syscall_count));
	init_tracing();
	enable_sys_enter();
	if (Trace_exit) {
		enable_sys_exit();
	}
	num_cpus = sysconf(_SC_NPROCESSORS_CONF);
	if (Dump) num_cpus = 1; // for now while playing with it
	args = ezalloc(num_cpus * sizeof(Collector_args_s));
	for (i = 0; i < num_cpus; i++, args++) {
		args->cpu_id = i;
		rc = pthread_create(&collector_thread, NULL,
			Dump ? dump_collector : collector, args);
		if (rc) fatal("Couldn't create collector %d:", rc);
	}
}
