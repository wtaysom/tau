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

#define CNTRL(_x_)	((_x_) & 0x1f)

extern int		Format;
extern ulong	Lastx;

static int		NewNumber;
static int		Escape;
static int		Radix;
//static int		NewRadix[] = { 16, 10, 10, 2, 256 };
static int		NewRadix[] = { 16, 10, 10, 8, 256 };

static void lastx (void)
{
	Lastx = ith(0);
	NewNumber = FALSE;
}

static void digit (int c, char offset)
{
	ulong	n;

	if (NewNumber)	{
		n = pop();
		n *= Radix;
		n += c - offset;
	} else {
		NewNumber = TRUE;
		n = c - offset;
	}
	push(n);
}

void input (void)
{
	int	c;

	Escape    = FALSE;
	NewNumber = TRUE;
	Radix     = 16;

	output(NewNumber);

	for (;;) {

		c = getch();

		if (Escape && (Format == CHARACTER)) {

			digit(c, 0);
			Escape = FALSE;

		} else switch (c) {

		case '_':	lastx();	neg();		break;
		case '+':	lastx();	add();		break;
		case '-':	lastx();	sub();		break;
		case '*':	lastx();	mul();		break;
		case '/':	lastx();	divide();	break;
		case '%':	lastx();	mod();		break;
		case '~':	lastx();	not();		break;
		case '&':	lastx();	and();		break;
		case '|':	lastx();	or();		break;
		case '^':	lastx();	xor();		break;
		case '<':	lastx();	leftShift();	break;
		case '>':	lastx();	rightShift();	break;
		case '!':	lastx();	store();	break;
		case '?':	lastx();	retrieve();	break;
		case '.':	lastx();	swap();		break;
		case '#':			lrand();	break;

		case KEY_RESIZE:
					break;

		case CNTRL('R'):	rotate();
					break;

		case CNTRL('U'):	push(Lastx);
					NewNumber = FALSE;
					break;

		case KEY_ENTER:
		case CNTRL('M'):	if (!NewNumber)	{
						dup();
					}
					NewNumber = FALSE;
					break;

		case CNTRL('I'):	++Format;
					if (Format > CHARACTER)	Format = HEX;
					Radix = NewRadix[Format];
					break;

		case KEY_DC:
		case CNTRL('H'):	push(pop() / Radix);
					break;

		case CNTRL('X'):	(void)pop();
					push(0);
					NewNumber = TRUE;
					break;

		case CNTRL('T'):	initStack();
					NewNumber = TRUE;
					break;

		case CNTRL('A'):	initStack();
					initMemory();
					Lastx = 0;
					NewNumber = TRUE;
					break;

		case KEY_CLEAR:
		case CNTRL('L'):	wrefresh(curscr);
					break;

		case CNTRL('D'):	finish(0);
					break;

		case CNTRL('V'):	if (Format == CHARACTER) Escape = TRUE;
					break;

		default:	if ((Format == HEX) && isxdigit(c)) {
					if (isdigit(c))     digit(c, '0');
					else if (isupper(c)) digit(c, 'A' - 10);
					else		     digit(c, 'a' - 10);
				} else if ((Format == UNSIGNED) && isdigit(c)) {
					digit(c, '0');
				} else if ((Format == SIGNED) && isdigit(c)) {
					digit(c, '0');
				} else if ((Format == OCTAL)
						&& isdigit(c)
						&& (c != '8' && c != '9')) {
					digit(c, '0');
				} else if (Format == CHARACTER)	{
					digit(c, 0);
				} else {
					flash();
				}
				break;
		}
		output(NewNumber);
	}
}
