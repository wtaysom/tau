/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <debug.h>
#include <style.h>

#include "ktop.h"
#include "plot.h"
#include "syscall.h"
#include "tickcounter.h"

enum {	HELP_ROW = 0,
	HELP_COL = 0,
	RW_ROW   = HELP_ROW + 1,
	RW_COL   = 0,
	PID_ROW  = RW_ROW + 6,
	PID_COL  = 0,
	TOP_ROW  = RW_ROW + 6,
	TOP_COL  = 70,
	SELF_ROW = HELP_ROW + 1,
	SELF_COL = 40 };

typedef struct Top_ten_s {
	int	syscall;
	int	value;
} Top_ten_s;

bool Plot = FALSE;

static u64 A[NUM_SYS_CALLS];
static u64 B[NUM_SYS_CALLS];
static u64 *Old = A;
static u64 *New = B;
static int Delta[NUM_SYS_CALLS];
static Top_ten_s Top_ten[10];

static TickCounter_s TotalDelta;
static graph_s TotalGraph = {{0, 0}, {{0, 10}, {60, 20}}};

static void help(void)
{
	mvprintw(HELP_ROW, HELP_COL,
		"q - quit  c - clear");
}

char *getpidname(int pid)
{
	char path[100];
	static char name[4096];
	int rc;

	snprintf(path, sizeof(path), "/proc/%d/exe", pid);
	rc = readlink(path, name, sizeof(name));
	if (rc == -1) return NULL;
	name[rc] = '\0';
	
	return name; //strdup(name);
}

static void read_write(void)
{
	mvprintw(RW_ROW,   RW_COL, "            total   hits/sec");
	mvprintw(RW_ROW+1, RW_COL, "read:  %10lld %10d",
		New[sys_read], Delta[sys_read]);
	mvprintw(RW_ROW+2, RW_COL, "write: %10lld %10d",
		New[sys_write], Delta[sys_write]);
	mvprintw(RW_ROW+3, RW_COL, "pread: %10lld %10d",
		New[sys_pread64], Delta[sys_pread64]);
	mvprintw(RW_ROW+4, RW_COL, "pwrite:%10lld %10d",
		New[sys_pwrite64], Delta[sys_pwrite64]);
}

static void self(void)
{
	mvprintw(SELF_ROW,   SELF_COL, "Skipped: %8lld", MyPidCount);
	mvprintw(SELF_ROW+1, SELF_COL, "Slept:   %8lld", Slept);
}

static void top_pid(void)
{
	int max = 0;
	int pid = 0;
	int i;

	for (i = 0; i < MAX_PID; i++) {
		if (Pid[i] > max) {
			max = Pid[i];
			pid = i;
		}
	}
	mvprintw(TOP_ROW, TOP_COL, "max: %d %d", pid, max);
}

static void display_pidcall(void)
{
	Pidcall_s *p = Pidcall;
	int row = PID_ROW;
	int col = PID_COL;
	int pid;
	int i;

	mvprintw(row++, col, "       pid  total");
	for (i = 0; i < 25; i++, p++, row++) {
		if (p == Pidnext) return;
		pid = get_pid(p->pidcall);
		mvprintw(row, col, "%3d. %5d %6d %-22.22s %-28.28s",
			i+1, pid, p->count,
			Syscall[get_call(p->pidcall)],
			getpidname(pid));
	}
}

static void display_top_ten(void)
{
	int row = TOP_ROW + 1;
	int i;

	mvprintw(row++, TOP_COL, "    hits sys_call");
	for (i = 0; i < 10; i++, row++) {
		if (Top_ten[i].value == 0) return;
		mvprintw(row, TOP_COL, "%8d %-22.22s",
			Top_ten[i].value, Syscall[Top_ten[i].syscall]);
	}
}

static void top_ten_insert(int x, int syscall)
{
	int i;

	for (i = 0; i < 10; i++) {
		if (x > Top_ten[i].value) {
			memmove( &Top_ten[i+1], &Top_ten[i],
				(10 - (i + 1)) * sizeof(Top_ten[i]));
			Top_ten[i].value = x;
			Top_ten[i].syscall = syscall;
			return;
		}
	}
}

static void top_ten(void)
{
	int x;
	int i;

	zero(Top_ten);
	for (i = 0; i < NUM_SYS_CALLS; i++) {
		x = Delta[i];
		if (x > Top_ten[9].value) {
			top_ten_insert(x, i);
		}
	}
}

static void delta(void)
{
	u64 *tmp;
	int sum;
	int i;

	tmp = Old;
	Old = New;
	New = tmp;

	memmove(New, Syscall_count, sizeof(Syscall_count));

	sum = 0;
	for (i = 0; i < NUM_SYS_CALLS; i++) {
		Delta[i] = New[i] - Old[i];
		sum += Delta[i];
	}
	tick(&TotalDelta, sum);
	top_ten();
}

static void graph_total(void)
{
	vector_s v = new_vector(WHEEL_SIZE);
	int i;
	int j;

	j = TotalDelta.itick;
	for (i = 0; i < WHEEL_SIZE; i++) {
		v.p[i].x = j;
		v.p[i].y = TotalDelta.tick[j];
		if (++j == WHEEL_SIZE) {
			j = 0;
		}
	}
	vplot(&TotalGraph, v, '*');	
}

static void init(void)
{
	initscr();
	cbreak();
	noecho();
	nonl();
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);

	init_counter(&TotalDelta, 1);
}

void *display(void *arg)
{
	struct timespec sleep = { 1, 0 };

	ignore_pid(gettid());
	init();
	for (;;) {
		if (Command) {
			return NULL;
		}
		delta();
		if (Plot) {
			clear();
			graph_total();
		} else {
			top_pid();
			display_pidcall();
			display_top_ten();
		}
		help();
		read_write();
		self();
		refresh();
		nanosleep(&sleep, NULL);
	}
}

void clear_display(void)
{
	zero(A);
	zero(B);
	zero(Pid);
	zero(Syscall_count);
	MyPidCount = 0;
	Slept = 0;
}

void cleanup_display(void)
{
	endwin();
}
