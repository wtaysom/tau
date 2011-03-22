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
#include <style.h>

#include "tracer.h"

int Command_tid;

int Command;

pid_t gettid(void) { return syscall(__NR_gettid); }

void cleanup(int sig)
{
PRd(sig);
	cleanup_collector();
	cleanup_display();
	exit(0);
}

void set_signals(void)
{
	signal(SIGHUP,	cleanup);
	signal(SIGINT,	cleanup);
	signal(SIGQUIT,	cleanup);
	signal(SIGTRAP,	cleanup);
	signal(SIGABRT,	cleanup);
	signal(SIGFPE,	cleanup);
	signal(SIGILL,	cleanup);
	signal(SIGBUS,	cleanup);
	signal(SIGSEGV,	cleanup);
	signal(SIGTSTP,	cleanup);
}

void init(void)
{
	set_signals();
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
		default:
			break;  // ignore
		}
	}
}

int main(int argc, char **argv)
{
	pthread_t collector_thread;
	pthread_t display_thread;
	int rc;

	init();
	
	Command_tid = gettid();
	
	rc = pthread_create(&collector_thread, NULL, collector, NULL);
	rc = pthread_create(&display_thread, NULL, display, NULL);
	
	commander();
	
	cleanup(0);
	return 0;
}
