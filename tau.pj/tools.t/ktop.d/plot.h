/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#ifndef _PLOT_H
#define _PLOT_H 1

typedef struct point_s {
	double	x;
	double	y;
} point_s;

typedef struct rect_s {
	point_s	min;
	point_s	max;
} rect_s;

typedef struct pixel_s {
	int	col;
	int	row;
} pixel_s;

typedef struct screen_s {
	pixel_s	start;
	pixel_s	range;
} screen_s;

typedef struct graph_s {
	point_s		max;
	screen_s	screen;
} graph_s;

typedef struct vector_s {
	int	n;
	point_s	*p;
} vector_s;

vector_s new_vector(int n);
void vplot(graph_s *g, vector_s v, char c);

#endif
