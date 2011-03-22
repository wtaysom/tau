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
#include "tracer.h"

#define _STR(x) #x
#define STR(x) _STR(x)
#define MAX_PATH 256

enum {	MAC_PATH = 256,
	BUF_SIZE = 1 << 12,	/* Page size seems to work best */
	SMALL_READ = BUF_SIZE >> 2 };

u64 Syscall_count[NUM_SYS_CALLS];
static int Trace_pipe;
int Collector_tid;
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

static void enable_sys_enter (void)
{
	FILE *enable;
	int rc;

	enable = fopen(tracing_file("events/syscalls/sys_enter/enable"), "w");
	if (!enable) {
		perror("events/syscalls/sys_enter/enable");
		exit(2);
	}
	rc = fprintf(enable, "1\n");
	if (rc < 0) {
		perror("enable");
		exit(2);
	}
	fclose(enable);
}

static void disable_sys_enter (void)
{
	FILE *enable;
	int rc;

	enable = fopen(tracing_file("events/syscalls/sys_enter/enable"), "w");
	if (!enable) {
		perror("events/syscalls/sys_enter/enable");
		exit(2);
	}
	rc = fprintf(enable, "0\n");
	if (rc < 0) {
		perror("enable");
		exit(2);
	}
	fclose(enable);
}

void dump_line(char *buf)
{
	int i;

	for (i = 0; buf[i]; i++) {
		fprintf(stderr, "%3d %c\n", i, buf[i]);
	}
	fprintf(stderr, "********\n");
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
		warn("Out of entries in pid/syscall table");
		return;
	}
	p = Pidnext++;
	p->pidcall = pidcall;
	p->count   = 1;	
}

/*
 * Example layout of data for sys_enter:
 *
 *           1         2         3         4         5         6         7         8         9         10
 * 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
 *           tracer-7181  [001]    35.953450: sys_enter: NR 4 (1, 77799000, 1, 77799000, 80ce0e0, 7f83921c)
 */

static void update_count(char *buf)
{
	int call_num = atoi(&buf[57]);
	int pid = atoi(&buf[17]);

	if (pid == Command_tid
	 || pid == Collector_tid
	 || pid == Display_tid) {
		++MyPidCount;
		return;
	}
	if (pid >= MAX_PID) {
		fatal("pid out of range %d", pid);
	}
	++Pid[pid];

	if (call_num >= Num_syscalls) {
		fprintf(stderr, "syscall number out of range %d\n", call_num);
		return;
	}
	++Syscall_count[call_num];
	record_pid_syscall(mkpidcall(pid, call_num));
#if 0
fprintf(stderr, "%s\n", buf);
fprintf(stderr, "%3d. %4lld %s\n",
	call_num, Syscall_count[call_num], Syscall[call_num]);
#endif
}

void break_into_lines(int n, char *buf)
{
	char *end = &buf[n];
	char *p;
	char *s = buf;

	for (p = buf; p < end; p++) {
		if (*p == '\n') {
			*p = '\n';
			update_count(s);
			s = p + 1;
		}
	}
}

static void init(void)
{
	Trace_pipe = open(tracing_file("trace_pipe"), O_RDONLY);
	if (Trace_pipe == -1) {
		perror("trace_pipe");
		exit(2);
	}
	enable_sys_enter();
}

void *collector(void *arg)
{
	char buf[BUF_SIZE];
	int rc;
	int i;

	Collector_tid = gettid();
	init();
	for (i = 0;; i++) {
		rc = read(Trace_pipe, buf, sizeof(buf));
		if (rc == -1) {
			cleanup(0);
		}
		break_into_lines(rc, buf);
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
}
