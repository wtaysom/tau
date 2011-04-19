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
#include <eprintf.h>
#include <qsort.h>
#include <style.h>

#include "ktop.h"
#include "plot.h"
#include "reduce.h"
#include "syscall.h"
#include "tickcounter.h"

enum {	HELP_ROW = 0,
	HELP_COL = 0,
	RW_ROW   = HELP_ROW + 1,
	RW_COL   = 0,
	PID_ROW  = RW_ROW + 8,
	PID_COL  = 0,
	SELF_ROW = HELP_ROW + 1,
	SELF_COL = HELP_ROW + 40,
	MAX_ROW  = SELF_ROW + 4,
	MAX_COL  = SELF_COL,
	TOP_ROW  = HELP_ROW,
	TOP_COL  = 90,
};

typedef struct Top_ten_s {
	int	syscall;
	int	value;
} Top_ten_s;

bool Plot = FALSE;

static Top_ten_s Top_ten[10];

static graph_s TotalGraph = {{0, 0}, {{0, 10}, {60, 20}}};

static void help(void)
{
	mvprintw(HELP_ROW, HELP_COL,
		"q quit  c clear  < shorter  > longer %d.%.3d",
		Sleep.tv_sec, Sleep.tv_nsec / A_MILLION);
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
	mvprintw(RW_ROW+5, RW_COL, "sync:  %10lld %10d",
		New[sys_sync], Delta[sys_sync]);
	mvprintw(RW_ROW+6, RW_COL, "fsync: %10lld %10d",
		New[sys_fsync], Delta[sys_fsync]);
}

static void self(void)
{
	static double max = 0;
	double avg;

	mvprintw(SELF_ROW,   SELF_COL, "Skipped: %8lld", MyPidCount);
	mvprintw(SELF_ROW+1, SELF_COL, "Slept:   %8lld", Slept);
	mvprintw(SELF_ROW+2, SELF_COL, "Tick:    %8zd", sizeof(TickCounter_s));
	mvprintw(SELF_ROW+3, SELF_COL, "No_enter:    %8lld", No_enter);
	mvprintw(SELF_ROW+4, SELF_COL, "Found:       %8lld", Found);
	mvprintw(SELF_ROW+5, SELF_COL, "Out_of_order:%8lld", Out_of_order);
	mvprintw(SELF_ROW+6, SELF_COL, "No_start:    %8lld", No_start);
	if (0) {
		if (PidcallRecord == 0) return;
		avg = (double)PidcallIterations / (double)PidcallRecord;
		PidcallIterations = PidcallRecord = 0;
		if (avg > max) max =avg;
		mvprintw(SELF_ROW+5, SELF_COL, "Avg:     %g", avg);
		mvprintw(SELF_ROW+6, SELF_COL, "Max:     %g", max);
	}
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
	mvprintw(MAX_ROW, MAX_COL, "max: %d %d", pid, max);
}

static char *getpidname(int pid)
{
	char path[100];
	static char name[4096];
	int rc;

	snprintf(path, sizeof(path), "/proc/%d/exe", pid);
	rc = readlink(path, name, sizeof(name));
	if (rc == -1) {
		return NULL;
	}
	if (rc == sizeof(name)) {
		fatal("pid name too long");
	}
	name[rc] = '\0';

	return strdup(name);
}

static void display_pidcall(void)
{
	Pidcall_s *pc;
	int row = PID_ROW;
	int col = PID_COL;
	int pid;
	int i;

	mvprintw(row++, col, "%3d    pid  count   duration", Num_rank);
	for (i = 0; i < 25 && i < Num_rank; i++, row++) {
		pc = Rank_pidcall[i];
		pid = get_pid(pc->pidcall);
		if (!pc->name) {
			pc->name = getpidname(pid);
			if (!pc->name) {
				pc->name = strdup("(unknown)");
			}
		}
		mvprintw(row, col, "%3d. %5d %6d %10lld %-22.22s %-28.28s",
			i + 1, pid, pc->save.count,
			pc->save.count ? pc->save.time / pc->save.count : 0LL,
			Syscall[get_call(pc->pidcall)],
			pc->name);
		mvprintw(row, col+80, "%8lx %8lx %8lx",
			pc->arg[0], pc->arg[1], pc->arg[2]);
	}
}

static void display_top_ten(void)
{
	int row = TOP_ROW;
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

void init_display(void)
{
	initscr();
	cbreak();
	noecho();
	nonl();
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);

	init_counter(&TotalDelta, 1);
}

void display(void)
{
	if (Plot) {
		clear();
		graph_total();
	} else {
		top_pid();
		display_pidcall();
		display_top_ten();
	}
	top_ten();
	help();
	read_write();
	self();
	refresh();
}

void cleanup_display(void)
{
	endwin();
}
