/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#include <float.h>
#include <math.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <eprintf.h>
#include <chart.h>
#include <debug.h>

typedef struct range_s {
	double	min;
	double	max;
} range_s;

typedef struct pixel_s {
	int	row;
	int	col;
} pixel_s;

struct chart_s {
	pixel_s	start;
	pixel_s	len;
	int	tick;
	double	ystep;
	int	ystepsize;
	int	ynumsteps;
	char	sym;
	bool	log;
	double	yscale;
	range_s	range;
	double	y[];
};

static void draw(chart_s *ch, int tick, double y, char sym)
{
	int	row;
	int	col;

	row = ch->len.row - round(ch->yscale * (y - ch->range.min));
	col = tick;
	row += ch->start.row;
	col += ch->start.col;
	mvprintw(row, col, "%c", sym);
}

void chart(chart_s *ch, double y)
{
	char	sym;

	if (ch->log) y = log10(y);
	if (y < ch->range.min) {
		y = ch->range.min;
		sym = 'v';
	} else if (y > ch->range.max) {
		y = ch->range.max;
		sym = '^';
	} else {
		sym = ch->sym;
	}
	/*if (ch->y[ch->tick])*/ draw(ch, ch->tick, ch->y[ch->tick], ' ');
	draw(ch, ch->tick, y, sym);
	refresh();
	ch->y[ch->tick++] = y;
	if (ch->tick == ch->len.col) ch->tick = 0;
}

static bool close_to_zero(double x, double limit)
{
	if (limit < 0) limit = - limit;
	limit /= 1000;
	return (x < limit) && (x > -limit);
}

static void draw_y_axis(chart_s *ch)
{
	int	step = ch->ystepsize;
	int	row;
	int	col;
	double	tick;

	for (row = ch->start.row; row <= ch->start.row + ch->len.row; row++) {
		mvprintw(row, ch->start.col - 1, "|");
	}
	tick = ch->range.min;
	if (ch->log && tick == 0.0) {
		tick = 1.0;
	}
	col  = 0;
	for (row = ch->start.row + ch->len.row;
	     row >= ch->start.row;
	     row -= step) {
		mvprintw(row, col, "%6g", tick);
		if (ch->log) {
			tick *= ch->ystep;
		} else {
			tick += ch->ystep;
		}
		if (close_to_zero(tick, ch->ystep)) {
			tick = 0;
		}
	}
}

static void draw_x_axis(chart_s *ch)
{
	int	col;

	for (col = ch->start.col; col < ch->start.col + ch->len.col; col++) {
		mvprintw(ch->start.row + ch->len.row + 1, col, "-");
	}
}

static void draw_origin(chart_s *ch)
{
	mvprintw(ch->start.row + ch->len.row + 1,  ch->start.col - 1, "+");
}

static void draw_chart(chart_s *ch)
{
	clear();
	draw_y_axis(ch);
	draw_x_axis(ch);
	draw_origin(ch);
	refresh();
}

chart_s *new_chart(
	int row_start,
	int col_start,
	int cols,
	double ymin,
	double ystep,
	int ystepsize,
	int ynumsteps,
	char sym,
	bool logarithmic)
{
	chart_s	*ch;
	double	ymax;
	int	rows = ystepsize * ynumsteps;

	ch = ezalloc(sizeof(*ch) + cols * sizeof(double));
	ch->start.row = row_start;
	ch->start.col = col_start;
	ch->len.row = rows;
	ch->len.col = cols;
	ch->tick = 0;
	if (logarithmic) {
		int	i;
		ymax = ymin;
		for (i = 0; i < ynumsteps; i++) {
			ymax *= ystep;
		}
		ymin = log10(ymin);
		ymax = log10(ymax);
		ch->yscale = (double)rows / (ymax - ymin);
		ch->range.min = (ymin);
		ch->range.max = (ymax);
	} else {
		ymax = ymin + ystep * ynumsteps;
		ch->yscale = (double)rows / (ymax - ymin);
		ch->range.min = ymin;
		ch->range.max = ymax;
	}
	ch->ystep = ystep;
	ch->ystepsize = ystepsize;
	ch->ynumsteps = ynumsteps;
	ch->sym  = sym;
	ch->log  = logarithmic;
	draw_chart(ch);
	return ch;
}
