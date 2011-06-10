/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
/*
 * Chart plots a point at each tick.
 */
#ifndef _CHART_H_
#define _CHART_H_ 1

typedef struct chart_s	chart_s;

void chart(chart_s *ch, double y);
chart_s *new_chart(
	int row_start,
	int col_start,
	int rows,
	int cols,
	double ymin,
	double ymax,
	char sym,
	bool logarithmic);

#endif
