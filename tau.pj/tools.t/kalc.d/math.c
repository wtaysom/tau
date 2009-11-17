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
#include <math.h>

#include "kalc.h"

void add (void)
{
	push(pop() + pop());
}

void sub (void)
{
	double	x = pop();

	push(pop() - x);
}

void mul (void)
{
	push(pop() * pop());
}

void divide (void)
{
	double	x = pop();

	if (x == 0) push(0);
	else push(pop() / x);
}

void power (void)
{
	double	x = pop();

	if (x == 0) push(1);
	else push(exp(log(pop()) * x));
}

void neg (void)
{
	push( -pop());
}

void dsqrt (void)
{
	push(sqrt(pop()));
}

void dsin (void)
{
	push(sin(pop()));
}

void dcos (void)
{
	push(cos(pop()));
}

void dtan (void)
{
	push(tan(pop()));
}

void drand (void)
{
	push(drand48());
}
