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

#include <string.h>

#include "qalc.h"

static uquad	Stack[STSIZE];

void initStack (void)
{
	bzero(Stack, sizeof(Stack));
}

uquad pop ()
{
	index_t	i;
	uquad	x = Stack[0];

	for (i = 0; i < STSIZE-1; ++i)	{
		Stack[i] = Stack[i+1];
	}
	return x;
}

void push (uquad x)
{
	index_t	i;

	for (i = STSIZE-1; i > 0; --i)	{
		Stack[i] = Stack[i-1];
	}
	Stack[0] = x;
}

void swap (void)
{
	uquad	x = Stack[0];

	Stack[0] = Stack[1];
	Stack[1] = x;
}

void rotate (void)
{
	index_t	i;
	uquad	x = Stack[0];

	for (i = 0; i < STSIZE-1; ++i)	{
		Stack[i] = Stack[i+1];
	}
	Stack[STSIZE-1] = x;
}

void dup (void)
{
	push(Stack[0]);
}

uquad ith (index_t i)
{
	if (i >= STSIZE)	return 0;
	return Stack[i];
}
