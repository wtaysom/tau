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
	char	sym;
	bool	logarithmic;
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
	if (ch->logarithmic) y = log10(y);
	draw(ch, ch->tick, ch->y[ch->tick], ' ');
	draw(ch, ch->tick, y, ch->sym);
	refresh();
	ch->y[ch->tick++] = y;
	if (ch->tick == ch->len.col) ch->tick = 0;
}

chart_s *new_chart(
	int row_start,
	int col_start,
	int rows,
	int cols,
	double ymin,
	double ymax,
	char sym,
	bool logarithmic)
{
	chart_s	*ch;

	ch = ezalloc(sizeof(*ch) + rows * sizeof(double));
	ch->start.row = 0;
	ch->start.col = 0;
	ch->len.row = rows;
	ch->len.col = cols;
	ch->tick = 0;
	ch->sym  = sym;
	ch->logarithmic = logarithmic;
	if (logarithmic) {
		ch->yscale = (double)rows / log10(ymax - ymin);
	} else {
		ch->yscale = (double)rows / (ymax - ymin);
	}
	ch->range.min = ymin;
	ch->range.max = ymax;
	return ch;
}
