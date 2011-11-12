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

bool Halt = FALSE;

bool Dump = FALSE;
bool Trace_exit = TRUE;
bool Trace_self = FALSE;
bool Pause = FALSE;
bool Help = FALSE;

Display_s Display;

pid_t gettid(void) { return syscall(__NR_gettid); }

u8 Ignore_pid[(MAX_PID + 4) / 8];
pthread_mutex_t Ignore_pid_lock = PTHREAD_MUTEX_INITIALIZER;
u64 Ignored_pid_count;

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
	if (Ignore_pid[pid / 8] & (1 << (pid & 0x7))) {
		++Ignored_pid_count;
		return TRUE;
	} else {
		return FALSE;
	}
}


void cleanup(int sig)
{
	cleanup_collector();
	cleanup_display();
	if (sig) {
		fatal("killed by signal %d\n", sig);
	}
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
	fprintf(stderr, "usage: %s [-dhs]\n"
		"\td - dump of ftrace log of cpu 0 for debugging\n"
		"\th - usage\n"
		"\ts - trace self\n\n"
		"\tCommands while running:\n"
		"\t? - help for current screen\n"
		"\tq - quit\n"
		"\tc - reset internal counters\n"
		"\tk - display top kernel operations (default)\n"
		"\tg - display graph of selected operation\n"
		"\tf - display just file system operations\n"
		"\ti - display counters internal to ktop for debugging\n"
		"\tp - toggle pause\n"
		"\t< - reduce redisplay interval\n"
		"\t> - increase redisplay interval\n",
		getprogname());
	exit(2);
}

static void init(int argc, char *argv[])
{
	int c;

	setprogname(argv[0]);
	set_signals();

	while ((c = getopt(argc, argv, "dhs")) != -1) {
		switch (c) {
		case 'd':
			Dump = TRUE;
			break;
		case 's':
			Trace_self = TRUE;
			break;
		case 'h':
			usage();
			break;
		default:
			fprintf(stderr, "unknown flag '%c'\n", c);
			usage();
			break;
		}
	}
	Display = Kernel_display;
}

void quit(void)
{
	Halt = TRUE;
}

void clear(void)
{
	reset_reduce();
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
			decrease_reduce_interval();
			break;
		case '>':
			increase_reduce_interval();
			break;
		case '?':
			Help = !Help;
			break;
		case 'i':
			Display = Internal_display;
			break;
		case 'k':
			Display = Kernel_display;
			break;
		case 'g':
			Display = Plot_display;
			break;
		case 'f':
			Display = File_system_display;
			break;
		case 'p':
			Pause = !Pause;
			break;
		default:
			Pause = FALSE;
			Help = FALSE;
			break;
		}
	}
}

int main(int argc, char **argv)
{
	pthread_t reduce_thread;
	int rc;

	debugstderr();

	init(argc, argv);

	ignore_pid(gettid());

	start_collector();

	if (!Dump) {
		rc = pthread_create(&reduce_thread, NULL, reduce, NULL);
		if (rc) fatal("creating reduce thread:");
	}
	commander();

	cleanup(0);
	return 0;
}
