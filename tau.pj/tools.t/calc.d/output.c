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
#include "calc.h"

extern int		Format;
extern ulong	Memory[];
extern ulong	Lastx;

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
	ulong	x;
	int	ch;

	for (i = 0; i < STSIZE; i++)	{

		if ((i == 0) && newNumber) attron(A_UNDERLINE);
		else			   attroff(A_UNDERLINE);

		x = ith(i);
		mvprintw(i, 1, "%8lx %10lu %11ld %11lo", x, x, x, x);
		addch(' ');
		for (j = 0; j < 4; ++j)	{
			ch = (x >> ((3 - j) * 8)) & 0377;

			if (!(isascii(ch))) { addch('!'); ch -= 0200; }
			else		      addch(' ');

			if (ch == 0200)  { addch('^'); ch = '~'; }
			if (iscntrl(ch)) { addch('^'); ch += 'A' - 1; }
			else		   addch(' ');

			addch(ch);
		}
	}
	mvprintw(8, 0, "last x: 0x%.8lx", Lastx);
	for (i = 0; i < MAX_MEM; ++i) {
		mvprintw(9, i*9+7, "%d", i);
		mvprintw(10, i*9, "%8x ", Memory[i]);
	}
	help();
	switch (Format)	{

	case HEX:	move(0,  8);	break;
	case SIGNED:	move(0, 19);	break;
	case UNSIGNED:	move(0, 31);	break;
	case OCTAL:	move(0, 43);	break;
	case CHARACTER:	move(0, 56);	break;
	}
	refresh();
}
