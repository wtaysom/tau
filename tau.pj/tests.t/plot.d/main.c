/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <float.h>
#include <math.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <eprintf.h>
#include <plot.h>
#include <chart.h>
#include <debug.h>

double f(double x)
{
	return sin(x/12);
}

#if 0

vector_s vinit(int n, double f(double))
{
	vector_s v = new_vector(n);
	point_s *p = v.p;
	point_s *end;
	double x;

	for (x = 0, end = &v.p[n]; p < end; p++, x++) {
		p->x = x;
		p->y = f(x);
	}
	return v;
}

void graph(void)
{
	graph_s g = { {0, 0}, { {10, 10}, {80, 10} } };
	vector_s v;

	v = vinit(80, f);
	vplot(&g, v, '*');
}
#endif

void chart1(void)
{
	enum { ROWS = 20, COLS = 132 };
	chart_s	*ch;
	int	i;

	ch = new_chart(3, 3, ROWS, COLS, -1.0, 1.0, '*', FALSE);
	
	for (i = 0; i < 1000; i++) {
		chart(ch, f(i));
		usleep(10000);
	}
}

void chart2(void)
{
	enum { ROWS = 20, COLS = 132 };
	chart_s	*ch;
	int	i;

	ch = new_chart(3, 3, ROWS, COLS, 0, 100000000, '@', TRUE);
	
	for (i = 0; i < 1000; i++) {
		chart(ch, i * i);
		usleep(10000);
	}
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

static void cleanup(int sig)
{
	endwin();
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

int main(int argc, char *argv[])
{
	debugstderr();
	set_signals();
	init();
	chart1();
	chart2();
	getchar();
	cleanup(0);
	return 0;
}
