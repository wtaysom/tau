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

void pr_point(point_s p)
{
	fprintf(stderr, "x=%g y=%g\n", p.x, p.y);
}

void pr_pixel(pixel_s p)
{
	fprintf(stderr, "colx=%d row=%d\n", p.col, p.row);
}

vector_s new_vector(int n)
{
	vector_s v;

	if (!n) {
		v.n = 0;
		v.p = NULL;
		return v;
	}
	v.p = ezalloc(n * sizeof(*v.p));
	v.n = n;
	return v;
}

static int x2col(graph_s *g, double x)
{
	return round((x * g->screen.range.col) / g->max.x) + g->screen.start.col;
}

static int y2row(graph_s *g, double y)
{
	int row = round(g->screen.range.row
		- ((y * g->screen.range.row) / g->max.y))
		+ g->screen.start.row;
	return row;
}

pixel_s scale(graph_s *g, point_s p)
{
	pixel_s	q = { x2col(g, p.x), y2row(g, p.y) };
	return q;
}

void plot(graph_s *g, point_s p, char c)
{
	pixel_s q = scale(g, p);

	mvprintw(q.row, q.col, "%c", c);
}

point_s vmax(vector_s v)
{
	point_s max = { DBL_MIN, DBL_MIN };
	point_s	*p = v.p;
	point_s *end;

	if (!p) return max;
	for (end = &v.p[v.n]; p < end; p++) {
		if (p->x > max.x) max.x = p->x;
		if (p->y > max.y) max.y = p->y;
	}
	return max;
}

void vplot(graph_s *g, vector_s v, char c)
{
	int i;

	g->max = vmax(v);
	for (i = 0; i < v.n; i++) {
		plot(g, v.p[i], c);
	}
}
