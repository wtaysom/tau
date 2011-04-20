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
#include <mylib.h>
#include <style.h>

#include "collector.h"
#include "ktop.h"
#include "syscall.h"

#define _STR(x) #x
#define STR(x) _STR(x)
#define MAX_PATH 256

enum { PIDCALL_BUCKETS = 1543 };

pthread_mutex_t Count_lock = PTHREAD_MUTEX_INITIALIZER;

u64 Syscall_count[NUM_SYS_CALLS];
u64 MyPidCount;
u64 Slept;
int Pid[MAX_PID];

Pidcall_s Pidcall[MAX_PIDCALLS];
Pidcall_s *Pidclock = Pidcall;
u64 PidcallIterations;
u64 PidcallRecord;
u64 Pidcall_tick;

u64 No_enter;
u64 Found;
u64 Out_of_order;
u64 No_start;

Pidcall_s *Pidcall_bucket[PIDCALL_BUCKETS];

static const char *find_debugfs(void)
{
       static char debugfs[MAX_PATH+1];
       static int debugfs_found;
       char type[100];
       FILE *fp;

       if (debugfs_found)
	       return debugfs;

       if ((fp = fopen("/proc/mounts","r")) == NULL) {
	       perror("/proc/mounts");
	       return NULL;
       }

       while (fscanf(fp, "%*s %"
		     STR(MAX_PATH)
		     "s %99s %*s %*d %*d\n",
		     debugfs, type) == 2) {
	       if (strcmp(type, "debugfs") == 0) break;
       }
       fclose(fp);

       if (strcmp(type, "debugfs") != 0) {
	       fprintf(stderr, "debugfs not mounted");
	       return NULL;
       }

       strcat(debugfs, "/tracing/");
       debugfs_found = 1;

       return debugfs;
}

static const char *tracing_file(const char *file_name)
{
	static char trace_file[MAX_PATH+1];
	snprintf(trace_file, MAX_PATH, "%s/%s", find_debugfs(), file_name);
	return trace_file;
}

static void enable_event(char *name)
{
	char file_name[MAX_PATH+1];
	FILE *file;
	int rc;

	snprintf(file_name, MAX_PATH, "events/syscalls/%s/enable", name);
	file = fopen(tracing_file(file_name), "w");
	if (!file) {
		perror(file_name);
		exit(2);
	}
	rc = fprintf(file, "1\n");
	if (rc < 0) {
		perror(file_name);
		exit(2);
	}
	fclose(file);
}

static void disable_event(char *name)
{
	char file_name[MAX_PATH+1];
	FILE *file;
	int rc;

	snprintf(file_name, MAX_PATH, "events/syscalls/%s/enable", name);
	file = fopen(tracing_file(file_name), "w");
	if (!file) {
		perror(file_name);
		exit(2);
	}
	rc = fprintf(file, "0\n");
	if (rc < 0) {
		perror(file_name);
		exit(2);
	}
	fclose(file);
}

static void enable_sys_enter(void)
{

	enable_event("sys_enter");
}

static void disable_sys_enter (void)
{
	disable_event("sys_enter");
}

static void enable_sys_exit(void)
{

	enable_event("sys_exit");
}

static void disable_sys_exit (void)
{
	disable_event("sys_exit");
}

int open_raw(int cpu)
{
	char name[MAX_NAME];
	int fd;

	snprintf(name, sizeof(name), "per_cpu/cpu%d/trace_pipe_raw", cpu);
	fd = open(tracing_file(name), O_RDONLY);
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

++PidcallRecord;
	for (;;) {
++PidcallIterations;
		pc = pc->next;
		if (!pc) return NULL;
		if (pc->pidcall == pidcall) {
			pc->clock = 1;
			return pc;
		}
	}
}

#if 0
static void dump(char *label, Pidcall_s *pc)
{
	int i;

	fprintf(stderr, "%s", label);
	for (i = 0; (i < 10) && pc; i++) {
		fprintf(stderr, " %p", pc);
		pc = pc->next;
	}
	if (pc) fprintf(stderr, "STUCK");
	fprintf(stderr, "\n");
}
#endif

static void add_pidcall(Pidcall_s *pidcall)
{
	Pidcall_s *pc = hash_pidcall(pidcall->pidcall);

	pidcall->next = pc->next;
	pc->next = pidcall;
}

static void rmv_pidcall(u32 pidcall)
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

static Pidcall_s *victim_pidcall(u32 pidcall)
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
		rmv_pidcall(pc->pidcall);
	}
	if (pc->name) {
		free(pc->name);
	}
	zero(*pc);
	pc->pidcall = pidcall;
	add_pidcall(pc);

	return pc;
}

static void parse_sys_enter(void *event, u64 time)
{
	sys_enter_s *sy = event;
	int   pid       = sy->ev.pid;
	snint call_num  = sy->id;
	u32   pidcall   = mkpidcall(pid, call_num);
	Pidcall_s *pc;

	++Pid[pid];

	if (call_num >= Num_syscalls) {
		warn("syscall number out of range %ld\n", call_num);
		return;
	}
	++Syscall_count[call_num];

	pc = find_pidcall(pidcall);
	if (!pc) {
		pc = victim_pidcall(pidcall);
	}
	++pc->count;
	pc->time.start = time;
	memmove(pc->arg, sy->arg, sizeof(pc->arg));
}

static void parse_sys_exit(void *event, u64 time)
{
	sys_exit_s *sy = event;
	int   pid      = sy->ev.pid;
	snint call_num = sy->id;
	u32   pidcall  = mkpidcall(pid, call_num);
	Pidcall_s *pc;

	pc = find_pidcall(pidcall);
	if (!pc) {
		++No_enter;
		return;
	}
	if (pc->time.start) {
		if (time > pc->time.start) {
			++Found;
			pc->time.total += time - pc->time.start;
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

	if (Trace_self) {
		if (!do_ignore_pid(event->pid)) {
			return;
		}
		++MyPidCount;
	} else {
		if (do_ignore_pid(event->pid)) {
			++MyPidCount;
			return;
		}
	}
	switch (event->type) {
	case 21:
		parse_sys_exit(event, time);
		break;
	case 22:
		parse_sys_enter(event, time);
		break;
	default:
		//printf(" no processing\n");
		break;
	}
}

static unint parse_buf(u8 *buf)
{
	ring_header_s *rh = (ring_header_s *)buf;
	ring_event_s *r;
	unint commit;
	unint length;
	unint size;
	u64 time;
	u8 *end;

	time = rh->time_stamp;
	commit = rh->commit;
	buf += sizeof(*rh);
	end  = &buf[commit];
	pthread_mutex_lock(&Count_lock);
	for (; buf < end; buf += size) {
		r = (ring_event_s *)buf;
		if (r->type_len == 0) {
			/* Larger record where size is at beginning of record */
			length = r->array[0];
			size   = 4 + length * 4;
			time  += r->time_delta;
		} else if (r->type_len <= 28) {
			/* Data record */
			length = r->type_len;
			size   = 4 + length * 4;
			time  += r->time_delta;
			parse_event(buf+4, time);
		} else if (r->type_len == 29) {
			/* Left over page padding or discarded event */
			if (r->time_delta == 0) {
				goto done;
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
			time = r->array[0];
			time += *(u64 *)&(r->array[1]) * A_BILLION;
		} else {
			warn(" Unknown event %d", r->type_len);
			/* Unknown - ignore */
			size = 4;
		}
	}
done:
	pthread_mutex_unlock(&Count_lock);
	return commit;
}

void *collector(void *args)
{
	Collector_args_s *a = args;
	/*
	 *  1 ms -> 7% overhead
	 * 10 ms -> 1% overhead
	 */
	struct timespec sleep = { 0, 10 * A_MILLION };
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
