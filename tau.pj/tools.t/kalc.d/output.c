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

#include <ctype.h>
#include <ncurses.h>
#include "kalc.h"

int	Format;
double	Lastx;

static char	*Help[] = {
	"_ change sign",
	"+ add",
	"- subtract",
	"* multiply",
	"/ divide",
	"^ power",
	"r sqrt",
	"s sin",
	"c cos",
	"t tan",
	"= swap",
	"! store",
	"? retrive",
	"# random",
	"p pi",
	"ENTER",
	"^R Rotate stack",
	"^U last x",
	"^V escape char",
	"^H clr digit",
	"^X clr entry",
	"^T zero stack",
	"^A clear all",
	"^L refresh",
	"^D finish",
	0
};

#define HELP_SIZE	20

static void help (void)
{
	int	x, y;
	char	**s;

	x = 0;
	y = 13;
	for (s = Help; *s != NULL; s++)	{
		mvaddstr(y, x, *s);
		x += HELP_SIZE;
		if (x > 80 - HELP_SIZE) {
			x = 0;
			++y;
		}
	}
}

void output (flag_t newNumber)
{
	index_t	i;
	double	x;

	for (i = 0; i < STSIZE; i++) {

		if ((i == 0) && newNumber) attron(A_UNDERLINE);
		else			   attroff(A_UNDERLINE);

		x = ith(i);
		mvprintw(i, 0, "%16.8g", x);
	}
	mvprintw(8, 0, "                        ");
	mvprintw(8, 0, "last x: %16.8g", Lastx);
	for (i = 0; i < MAX_MEM; ++i) {
		mvprintw(9, i*17+15, "%d", i);
		mvprintw(10, i*17, "%16.8g ", Memory[i]);
	}
	help();
	move(0, 15);
	refresh();
}
