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

#include <stdlib.h>
#include <ncurses.h>
#include "kalc.h"

void init (void)
{
	setSignals();

	initscr();
	cbreak();
	noecho();
	nonl();
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);

	initStack();
	initMemory();

	srand48(37);
}
