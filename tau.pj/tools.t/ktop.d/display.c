/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <debug.h>
#include <style.h>

#include "syscall.h"
#include "tracer.h"

enum {	RW_ROW = 0,
	RW_COL = 0,
	PID_ROW = 5,
	PID_COL = 0,
	TOP_ROW = 10,
	TOP_COL = 90,
	SELF_ROW = 0,
	SELF_COL = 40 };

typedef struct Top_ten_s {
	int	syscall;
	int	value;
} Top_ten_s;

static u64 A[NUM_SYS_CALLS];
static u64 B[NUM_SYS_CALLS];
static u64 *Old = A;
static u64 *New = B;
static int Delta[NUM_SYS_CALLS];
static Top_ten_s Top_ten[10];

static void read_write(void)
{
	mvprintw(RW_ROW,   RW_COL, "read:  %10lld %10d",
		New[sys_read], Delta[sys_read]);
	mvprintw(RW_ROW+1, RW_COL, "write: %10lld %10d",
		New[sys_write], Delta[sys_write]);
	mvprintw(RW_ROW+2, RW_COL, "pread: %10lld %10d",
		New[sys_pread64], Delta[sys_pread64]);
	mvprintw(RW_ROW+3, RW_COL, "pwrite:%10lld %10d",
		New[sys_pwrite64], Delta[sys_pwrite64]);
}

static void self(void)
{
	mvprintw(SELF_ROW,   SELF_COL, "pid:   %10lld", MyPidCount);
	mvprintw(SELF_ROW+1, SELF_COL, "Slept: %10lld", Slept);
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
	int i;

	for (i = 0; i < 25; i++, p++) {
		if (p == Pidnext) return;
		mvprintw(PID_ROW+i, PID_COL, "%3d. %5d %6d %-22s",
			i, get_pid(p->pidcall), p->count,
			Syscall[get_call(p->pidcall)]);
	}
}

static void display_top_ten(void)
{
	int row = TOP_ROW + 1;
	int i;

	for (i = 0; i < 10; i++, row++) {
		if (Top_ten[i].value == 0) return;
		mvprintw(row, TOP_COL, "%8d %-22s",
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

static void process(void)
{
	u64 *tmp;
	int i;

	tmp = Old;
	Old = New;
	New = tmp;

	memmove(New, Syscall_count, sizeof(Syscall_count));

	for (i = 0; i < NUM_SYS_CALLS; i++) {
		Delta[i] = New[i] - Old[i];
	}
	top_ten();
}
	
static void init(void)
{
	initscr();
	cbreak();
	noecho();
	nonl();
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);
}

void *display(void *arg)
{
	ignore_pid(gettid());
	init();	
	for (;;) {
		if (Command) {
			return NULL;
		}
		process();
		read_write();
		top_pid();
		display_pidcall();
		display_top_ten();
		self();
		refresh();
		sleep(1);
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
