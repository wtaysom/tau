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
#include "qalc.h"

extern int		Format;
extern uquad	Memory[];
extern uquad	Lastx;

static char	*Help[] = {
	"_ change sign",
	"+ add",
	"- subtract",
	"* multiply",
	"/ divide",
	"% modulo",
	"~ not",
	"& and",
	"| or",
	"^ xor",
	"< left shift",
	"> right shift",
	". swap",
	"! store",
	"? retrive",
	"# random",
	"ENTER",
	"tab change base",
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
	index_t	j;
	uquad	x;
	int		ch;

	for (i = 0; i < STSIZE; i++)	{

		if ((i == 0) && newNumber) attron(A_UNDERLINE);
		else			   attroff(A_UNDERLINE);

		x = ith(i);
		mvprintw(i, 0, "%16qx %20qu %20qd ", x, x, x);
		addch(' ');
		for (j = 0; j < 8; ++j)	{
			ch = (x >> ((7 - j) * 8)) & 0377;
			if (iscntrl(ch))	{
				if (ch == 0) {
					addch(' ');
					ch = '.';
				} else {
					addch('^');
					ch += 'A' - 1;
				}
			} else if (isascii(ch)) {
				addch(' ');
			} else {
				addch(' ');
				ch = '.';
			}
			addch(ch);
		}
	}
	mvprintw(8, 0, "                        ");
	mvprintw(8, 0, "last x: %qx", Lastx);
	for (i = 0; i < MAX_MEM; ++i)	{
		mvprintw(9, i*17+15, "%d", i);
		mvprintw(10, i*17, "%16qx ", Memory[i]);
	}
	help();
	switch (Format)	{

	case HEX:		move(0,  15);	break;
	case SIGNED:	move(0, 36);	break;
	case UNSIGNED:	move(0, 57);	break;
	case CHARACTER:	move(0, 75);	break;
	}
	refresh();
}
