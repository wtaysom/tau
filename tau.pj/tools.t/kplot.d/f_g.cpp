/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <QtGui>
#include "plotter.h"

#include <math.h>

#include <style.h>

double f (double x, int id)
{
	unused(id);
	x /= 10;
	return 4 * x * sin(x);
}

double g (double x, int id)
{
	unused(id);
	x /= 10;
	return 4 * x * cos(x);
}

void start_f_g  (Plotter &plotter)
{
	int	color = 0;

	plotter.setWindowTitle(QObject::tr("f vs g"));
	plotter.setCurveData(color++, f);
	plotter.setCurveData(color++, g);
	plotter.show();
}
