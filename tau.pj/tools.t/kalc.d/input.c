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
#include <math.h>

#include "kalc.h"

#define CNTRL(_x_)	((_x_) & 0x1f)

static int	NewNumber;
static int	Escape;
static int	Decimal;

static void lastx (void)
{
	Lastx     = ith(0);
	NewNumber = FALSE;
	Decimal   = 0;
}

static void digit (int c)
{
	double	x;

	x = c - '0';
	if (Decimal) {
		x /= Decimal;
		Decimal *= 10;
		if (NewNumber)	{
			x += pop();
		}
	} else {
		if (NewNumber)	{
			x += pop() * 10.0;
		}
	}
	NewNumber = TRUE;
	push(x);
}

void input (void)
{
	int	c;

	Escape    = FALSE;
	NewNumber = TRUE;
	Decimal   = 0;

	output(NewNumber);

	for (;;) {

		c = getch();

		switch (c) {

		case '_':	lastx();	neg();		break;
		case '+':	lastx();	add();		break;
		case '-':	lastx();	sub();		break;
		case '*':	lastx();	mul();		break;
		case '/':	lastx();	divide();	break;
		case '^':	lastx();	power();	break;
		case 'r':	lastx();	dsqrt();	break;
		case 's':	lastx();	dsin();		break;
		case 'c':	lastx();	dcos();		break;
		case 't':	lastx();	dtan();		break;
		case '!':	lastx();	store();	break;
		case '?':	lastx();	retrieve();	break;
		case '=':	lastx();	swap();		break;
		case '#':			drand();	break;
		case 'p':			push(M_PI);	break;

		case '.':		Decimal = 10;
					break;

		case KEY_RESIZE:	break;

		case CNTRL('R'):	rotate();
					Decimal = 0;
					break;

		case CNTRL('U'):	push(Lastx);
					NewNumber = FALSE;
					Decimal = 0;
					break;

		case KEY_ENTER:
		case CNTRL('M'):	if (!NewNumber)	{
						dup();
					}
					NewNumber = FALSE;
					Decimal = 0;
					break;

		case KEY_DC:
		case CNTRL('H'):	push(pop() / 10);
					Decimal = 0;
					break;

		case CNTRL('X'):	(void)pop();
					push(0);
					NewNumber = TRUE;
					Decimal = 0;
					break;

		case CNTRL('T'):	initStack();
					NewNumber = TRUE;
					Decimal = 0;
					break;

		case CNTRL('A'):	initStack();
					initMemory();
					Lastx = 0;
					NewNumber = TRUE;
					Decimal = 0;
					break;

		case KEY_CLEAR:
		case CNTRL('L'):	wrefresh(curscr);
					break;

		case CNTRL('D'):	finish(0);
					break;

		default:	if (isdigit(c)) {
					digit(c);
				} else {
					flash();
				}
				break;

		}
		output(NewNumber);
	}
}
