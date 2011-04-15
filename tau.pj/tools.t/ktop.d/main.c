/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */


#include <sys/syscall.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "ktop.h"

int Command;

bool Dump = FALSE;
bool Trace_exit = TRUE;
bool Trace_self = FALSE;

pid_t gettid(void) { return syscall(__NR_gettid); }

u8 Ignore_pid[(MAX_PID + 4) / 8];
pthread_mutex_t Ignore_pid_lock = PTHREAD_MUTEX_INITIALIZER;

void ignore_pid(int pid)
{
	if ((pid < 0) || (pid >= MAX_PID)) warn("pid out of range %d", pid);

	pthread_mutex_lock(&Ignore_pid_lock);
	Ignore_pid[pid / 8] |= (1 << (pid & 0x7));
	pthread_mutex_unlock(&Ignore_pid_lock);
}

bool do_ignore_pid(int pid)
{
	if ((pid < 0) || (pid >= MAX_PID)) {
		warn("pid out of range %d", pid);
		return TRUE;
	}
	return Ignore_pid[pid / 8] & (1 << (pid & 0x7));
}


void cleanup(int sig)
{
PRd(sig);
	if (sig) {
		fprintf(stderr, "died because %d\n", sig);
	}
	cleanup_collector();
	cleanup_display();
	exit(0);
}

void set_signals(void)
{
	signal(SIGHUP,	cleanup);
	signal(SIGINT,	cleanup);
	signal(SIGQUIT,	cleanup);
	signal(SIGILL,	cleanup);
	signal(SIGTRAP,	cleanup);
	signal(SIGABRT,	cleanup);
	signal(SIGBUS,	cleanup);
	signal(SIGFPE,	cleanup);
	signal(SIGKILL,	cleanup);
	signal(SIGSEGV,	cleanup);
	signal(SIGPIPE,	cleanup);
	signal(SIGSTOP,	cleanup);
	signal(SIGTSTP,	cleanup);
}

static void usage(void)
{
	fprintf(stderr, "usage: %s [-ds?]\n"
		"\td - dump of ftrace log of cpu 0 for debugging\n"
		"\ts - trace self\n"
		"\t? - usage\n",
		getprogname());
	exit(2);
}

static void init(int argc, char *argv[])
{
	int c;

	setprogname(argv[0]);
	set_signals();

	while ((c = getopt(argc, argv, "ds?")) != -1) {
		switch (c) {
		case 'd':
			Dump = TRUE;
			break;
		case 's':
			Trace_self = TRUE;
			break;
		case '?':
			usage();
			break;
		default:
			fprintf(stderr, "unknown option %c\n", c);
			usage();
			break;
		}
	}
}

void quit(void)
{
	Command = 1;
}

void clear(void)
{
	clear_display();
}

void commander(void)
{
	for (;;) {
		int c = getchar();
		if (c == EOF) cleanup(0);
		switch (c) {
		case 'q':
			quit();
			return;
		case 'c':
			clear();
			break;
		case '<':
			decrease_display_interval();
			break;
		case '>':
			increase_display_interval();
			break;
		default:
			break;  // ignore
		}
	}
}

int main(int argc, char **argv)
{
	pthread_t display_thread;
	int rc;

	debugstderr();

	init(argc, argv);

	ignore_pid(gettid());

	start_collector();

	if (!Dump) {
		rc = pthread_create(&display_thread, NULL, display, NULL);
	}
	commander();

	cleanup(0);
	return 0;
}
