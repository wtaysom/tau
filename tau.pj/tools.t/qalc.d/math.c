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

#include "qalc.h"

void add (void)
{
	push(pop() + pop());
}

void sub (void)
{
	uquad	n = pop();

	push(pop() - n);
}

void mul (void)
{
	push(pop() * pop());
}

void divide (void)
{
	uquad	n = pop();

	if (n == 0)	push(0);
	else push(pop() / n);
}

void mod (void)
{
	uquad	n = pop();

	if (n == 0)	push(0);
	else push(pop() % n);
}

void neg (void)
{
	push( -pop());
}

void not (void)
{
	push( ~pop());
}

void and (void)
{
	push(pop() & pop());
}

void or (void)
{
	push(pop() | pop());
}

void xor (void)
{
	push(pop() ^ pop());
}

void leftShift (void)
{
	push(pop() << 1);
}

void rightShift (void)
{
	push(pop() >> 1);
}

void qrand (void)
{
	uquad	x;

	x = random();
	x <<= 46;
	x += random();
	x <<= 32;
	x += random();
	x <<= 16;
	x += random();
	push(x);
}
