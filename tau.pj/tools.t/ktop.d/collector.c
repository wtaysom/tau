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
#include <style.h>

#include "syscall.h"
#include "ktop.h"

#define _STR(x) #x
#define STR(x) _STR(x)
#define MAX_PATH 256

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


pthread_mutex_t Count_lock = PTHREAD_MUTEX_INITIALIZER;

u64 Syscall_count[NUM_SYS_CALLS];
u64 MyPidCount;
u64 Slept;
int Pid[MAX_PID];

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

static int init_raw(int cpu)
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

Pidcall_s Pidcall[MAX_PIDCALLS];
Pidcall_s *Pidnext = Pidcall;

static inline void swap_pidcall(Pidcall_s *p)
{
	Pidcall_s tmp;

	if (p == Pidcall) return;
	tmp = *p;
	*p = p[-1];
	p[-1] = tmp;
}

void record_pid_syscall (u32 pidcall)
{
	Pidcall_s *p;

	for (p = Pidcall; p < Pidnext; p++) {
		if (p->pidcall == pidcall) {
			++p->count;
			swap_pidcall(p);
			return;
		}
	}
	if (Pidnext == &Pidcall[MAX_PIDCALLS]) {
		--p;
		p->pidcall = pidcall;
		p->count   = 1;
		swap_pidcall(p);
		return;
	}
	p = Pidnext++;
	p->pidcall = pidcall;
	p->count   = 1;
}

static void process_sys_enter(void *event)
{
	sys_enter_s *sy = event;
	int pid = sy->ev.pid;
	snint call_num = sy->id;

	++Pid[pid];

	if (call_num >= Num_syscalls) {
		warn("syscall number out of range %ld\n", call_num);
		return;
	}
	++Syscall_count[call_num];
	record_pid_syscall(mkpidcall(pid, call_num));
}

static void process_sys_exit(void *event)
{
//	sys_exit_s *sy = event;
}

static void process_event(void *buf)
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
		process_sys_exit(event);
		break;
	case 22:
		process_sys_enter(event);
		break;
	default:
		//printf(" no processing\n");
		break;
	}
}

static unint process_buf(u8 *buf)
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
			process_event(buf+4);
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
			//tv_nsec = r->array[0];
			//tv_sec  = *(u64 *)&(r->array[1]);
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
	trace_pipe = init_raw(cpu);
	for (i = 0;; i++) {
		rc = read(trace_pipe, buf, sizeof(buf));
		if (rc == -1) {
			close(trace_pipe);
			cleanup(0);
		}
		rc = process_buf(buf);
		if (rc < SMALL_READ) {
			++Slept;
			nanosleep(&sleep, NULL);
		//	sleep(1);	// Wait for input to accumulate
		}
	}
	return NULL;
}

static void *dump_collector(void *args)
{
	Collector_args_s *a = args;
	u8 buf[BUF_SIZE];
	int cpu = a->cpu_id;
	int trace_pipe;
	int rc;
	int i;

	ignore_pid(gettid());
	trace_pipe = init_raw(cpu);
	for (i = 0;; i++) {
		rc = read(trace_pipe, buf, sizeof(buf));
		printf("i=%d rc=%d\n", i, rc);
		if (rc == -1) {
			close(trace_pipe);
			cleanup(0);
		}
		dump_raw(cpu, rc, buf);
		if (rc < SMALL_READ) {
			++Slept;
			sleep(1);	// Wait for input to accumulate
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
