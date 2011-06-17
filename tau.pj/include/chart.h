/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
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
	int cols,
	double ymin,
	double ystep,
	int ystepsize,
	int ynumsteps,
	char sym,
	bool logarithmic);

#endif
