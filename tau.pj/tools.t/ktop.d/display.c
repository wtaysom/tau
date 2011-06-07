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
#include <style.h>

#include "ktop.h"
#include "plot.h"
#include "reduce.h"
#include "syscall.h"
#include "tickcounter.h"

enum {	HELP_ROW = 0,
	HELP_COL = 0,
	TOP_ROW  = HELP_ROW + 1,
	TOP_COL  = 0,
	RW_ROW   = HELP_ROW + 1,
	RW_COL   = 0,
	PID_ROW  = TOP_ROW + 12,
	PID_COL  = 0,
	SELF_ROW = HELP_ROW + 1,
	SELF_COL = 0,
	MAX_ROW  = SELF_ROW,
	MAX_COL  = SELF_COL + 40,
};

typedef struct Top_ten_s {
	int	syscall;
	int	value;
} Top_ten_s;

typedef struct Display_call_s {
	char	*name;
	int	index;
} Display_call_s;

bool Plot = FALSE;

static Top_ten_s Top_ten[10];

static graph_s TotalGraph = {{0, 0}, {{0, 10}, {60, 20}}};

Display_call_s Display_call[] = {
	{ "read:  ", sys_read },
	{ "write: ", sys_write },
	{ "pread: ", sys_pread64 },
	{ "pwrite:", sys_pwrite64 },
	{ "sync:  ", sys_sync },
	{ "fsync: ", sys_fsync },
	{ "open:  ", sys_open },
	{ "close: ", sys_close },
	{ "creat: ", sys_creat },
	{ "unlink:", sys_unlink },
	{ "stat:  ", sys_stat64 },
	{ "fstat: ", sys_fstat64 },
	{ "fork:  ", ptregs_fork },
	{ "vfork: ", ptregs_vfork },
	{ NULL, 0 }};

static void help(void)
{
	mvprintw(HELP_ROW, HELP_COL,
		"q quit  c clear  k kernel ops  i internal ops  f file ops "
		" < shorter  > longer %d.%.3d",
		Sleep.tv_sec, Sleep.tv_nsec / A_MILLION);
}

static void file_system(void)
{
	Display_call_s *d;
	int row = RW_ROW;

	mvprintw(row,   RW_COL, "            total   hits/sec");

	for (d = Display_call; d->name; d++) {
		mvprintw(++row, RW_COL, "%s %10lld %10d",
			d->name, New[d->index], Delta[d->index]);
	}
}

static void self(void)
{
	static double max = 0;
	double avg;

	mvprintw(SELF_ROW,   SELF_COL, "Skipped:     %12lld", MyPidCount);
	mvprintw(SELF_ROW+1, SELF_COL, "Slept:       %12lld", Slept);
	mvprintw(SELF_ROW+2, SELF_COL, "Tick:        %12zd", sizeof(TickCounter_s));
	mvprintw(SELF_ROW+3, SELF_COL, "No_enter:    %12lld", No_enter);
	mvprintw(SELF_ROW+4, SELF_COL, "Found:       %12lld", Found);
	mvprintw(SELF_ROW+5, SELF_COL, "Out_of_order:%12lld", Out_of_order);
	mvprintw(SELF_ROW+6, SELF_COL, "No_start:    %12lld", No_start);
	if (1) {
		mvprintw(SELF_ROW+7, SELF_COL, "Ticks:       %12lld", Pidcall_tick);
		if (PidcallRecord == 0) return;
		avg = (double)PidcallIterations / (double)PidcallRecord;
		PidcallIterations = PidcallRecord = 0;
		if (avg > max) max =avg;
		mvprintw(SELF_ROW+8, SELF_COL, "Avg:              %g", avg);
		mvprintw(SELF_ROW+9, SELF_COL, "Max:              %g", max);
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

void kernel_display(void)
{
	clear();
	top_ten();
	if (Plot) {
		clear();
		graph_total();
	} else {
		display_pidcall();
		display_top_ten();
	}
	help();
	refresh();
}

void internal_display(void)
{
	clear();
	help();
	self();
	top_pid();
	refresh();
}

void file_system_display(void)
{
	clear();
	help();
	file_system();
	refresh();
}

void cleanup_display(void)
{
	endwin();
}
