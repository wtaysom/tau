/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <sys/syscall.h>
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
	TOP_PID_CALL_ROW = TOP_ROW,
	TOP_PID_CALL_COL = 32,
};

typedef struct Top_ten_s {
	int	syscall;
	int	value;
} Top_ten_s;

typedef struct Display_call_s {
	char	*name;
	int	index;
} Display_call_s;

/* Top ten highest count for pid/sys_call */
static Top_ten_s Top_ten[10];

/* Defines graph area for total system calls */
static graph_s TotalGraph = {{0, 0}, {{0, 10}, {60, 20}}};

Display_call_s Display_call[] = {
	{ "read:  ", __NR_read },
	{ "write: ", __NR_write },
	{ "pread: ", __NR_pread64 },
	{ "pwrite:", __NR_pwrite64 },
	{ "sync:  ", __NR_sync },
	{ "fsync: ", __NR_fsync },
	{ "open:  ", __NR_open },
	{ "close: ", __NR_close },
	{ "creat: ", __NR_creat },
	{ "unlink:", __NR_unlink },
#ifdef __x86_64__
	{ "stat:  ", __NR_stat },
	{ "fstat: ", __NR_fstat },
	{ "lstat: ", __NR_lstat },
#else
	{ "stat:  ", __NR_stat64 },
	{ "fstat: ", __NR_fstat64 },
#endif
	{ NULL, 0 }};

static void help(void)
{
	mvprintw(HELP_ROW, HELP_COL,
		"? help  q quit  c clear  k kernel ops  g graph"
		"  i internal ops  f file ops"
		"  p pause"
		"  < shorter  > longer %d.%.3d",
		Sleep.tv_sec, Sleep.tv_nsec / ONE_MILLION);
}

static void file_system(void)
{
	Display_call_s *d;
	int row = RW_ROW;

	mvprintw(row,   RW_COL, "            total   calls/sec");

	for (d = Display_call; d->name; d++) {
		mvprintw(++row, RW_COL, "%s %10lld %10d",
			d->name, New[d->index], Delta[d->index]);
	}
}

static void self(void)
{
	static double max = 0;
	double avg;

	mvprintw(SELF_ROW,   SELF_COL, "Skipped:     %12lld", Ignored_pid_count);
	mvprintw(SELF_ROW+1, SELF_COL, "Slept:       %12lld", Slept);
	mvprintw(SELF_ROW+2, SELF_COL, "Tick:        %12zd", sizeof(TickCounter_s));
	mvprintw(SELF_ROW+3, SELF_COL, "No_enter:    %12lld", No_enter);
	mvprintw(SELF_ROW+4, SELF_COL, "Found:       %12lld", Found);
	mvprintw(SELF_ROW+5, SELF_COL, "Out_of_order:%12lld", Out_of_order);
	mvprintw(SELF_ROW+6, SELF_COL, "No_start:    %12lld", No_start);
	mvprintw(SELF_ROW+7, SELF_COL, "Bad type:    %12lld", Bad_type);
	if (1) {
		mvprintw(SELF_ROW+10, SELF_COL, "Ticks:       %12lld", Pidcall_tick);
		if (Pidcall_record == 0) return;
		avg = (double)Pidcall_iterations / (double)Pidcall_record;
		Pidcall_iterations = Pidcall_record = 0;
		if (avg > max) max =avg;
		mvprintw(SELF_ROW+11, SELF_COL, "Avg:              %g", avg);
		mvprintw(SELF_ROW+12, SELF_COL, "Max:              %g", max);
	}
	mvprintw(SELF_ROW+14, SELF_COL, "COLS:       %12d", COLS);
	mvprintw(SELF_ROW+15, SELF_COL, "LINES:      %12d", LINES);

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

	mvprintw(row++, col, "%3d    pid  calls   duration", Num_rank);
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
	}
}

static void display_top_pidcall(void)
{
	TopPidcall_s *tc;
	int row = TOP_PID_CALL_ROW;
	int col = TOP_PID_CALL_COL;

	mvprintw(row++, col, "|     calls   duration   when    pid");
	for (tc = Top_pidcall; tc < &Top_pidcall[MAX_TOP]; tc++, row++) {
		if (tc->count == 0) return;
		mvprintw(row, col,
		         "|  %8d %10lld %6d %6d %-22.22s %-30.30s",
		         tc->count, tc->time / tc->count,
		         tc->tick, get_pid(tc->pidcall),
		         Syscall[get_call(tc->pidcall)],
		         tc->name);
	}
}

static void display_top_ten(void)
{
	int row = TOP_ROW;
	int i;

	mvprintw(row++, TOP_COL, "   calls system call");
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
	for (i = 0; i < Num_syscalls; i++) {
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

	j = Total_delta.itick;
	for (i = 0; i < WHEEL_SIZE; i++) {
		v.p[i].x = j;
		v.p[i].y = Total_delta.tick[j];
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

	init_counter(&Total_delta, 1);
}

void kernel_display(void)
{
	clear();
	top_ten();
	display_pidcall();
	display_top_ten();
	display_top_pidcall();
	help();
	refresh();
}

void plot_display(void)
{
	clear();
	top_ten();
	graph_total();
	help();
	refresh();
}

void internal_display(void)
{
	clear();
	self();
	top_pid();
	help();
	refresh();
}

void file_system_display(void)
{
	clear();
	file_system();
	help();
	refresh();
}

void plot_help(void)
{
	int	row = HELP_ROW + 1;
	int	col = HELP_COL + 2;

	clear();
	help();
	row += 2;
	mvprintw(row, col, "Still playing.");
	row += 2;
	mvprintw(row, col, "Type <return> to get back to screen.");
	refresh();
}

void kernel_help (void)
{
	int	row = HELP_ROW + 3;
	int	col = HELP_COL + 2;

	clear();
	mvprintw(row, col, "Top 10 most frequent system" );
	++row;
	if (Sleep.tv_sec == 1 && Sleep.tv_nsec == 0) {
		mvprintw(row, col, "calls in last second.");
	} else {
		mvprintw(row, col, "calls in last %d.%.3d seconds.",
			Sleep.tv_sec, Sleep.tv_nsec / ONE_MILLION);
	}

	row += 10;
	mvprintw(row, col, "Most frequent system" );
	++row;
	if (Sleep.tv_sec == 1 && Sleep.tv_nsec == 0) {
		mvprintw(row, col, "calls in last second");
	} else {
		mvprintw(row, col, "calls in last %d.%.3d seconds",
			Sleep.tv_sec, Sleep.tv_nsec / ONE_MILLION);
	}
	++row;
	mvprintw(row, col, "by process.");

	row += 5;
	mvprintw(row, col, "Type <return> to return to screen");

	row = HELP_ROW + 3;
	col = TOP_PID_CALL_COL + 10;
	mvprintw(row, col, "All time top 10 most frequent" );
	++row;
	mvprintw(row, col, "system calls by process." );

	help();
	refresh();
}

void file_system_help (void)
{
	int	row = HELP_ROW + 3;
	int	col = HELP_COL + 2;

	clear();
	mvprintw(row, col, "File system calls ordered by name." );
	++row;
	if (Sleep.tv_sec == 1 && Sleep.tv_nsec == 0) {
		mvprintw(row, col,
			"Total calls made and calls in last second.");
	} else {
		mvprintw(row, col,
			"Total calls made and calls in last %d.%.3d seconds.",
			Sleep.tv_sec, Sleep.tv_nsec / ONE_MILLION);
	}
	row += 5;
	mvprintw(row, col, "Type <return> to return to screen");

	help();
	refresh();
}

void internal_help (void)
{
	int	row = HELP_ROW + 1;
	int	col = HELP_COL + 2;

	clear();
	row += 2;
	mvprintw(row, col, "Diplays what is going on inside ktop.");
	row += 1;
	mvprintw(row, col, "Used to debug ktop.");
	row += 2;
	mvprintw(row, col, "Type <return> to get back to screen.");
	help();
	refresh();
}

void cleanup_display(void)
{
	endwin();
}


Display_s Kernel_display      = { kernel_display,      kernel_help };
Display_s Internal_display    = { internal_display,    internal_help };
Display_s Plot_display        = { plot_display,        plot_help };
Display_s File_system_display = { file_system_display, file_system_help };
