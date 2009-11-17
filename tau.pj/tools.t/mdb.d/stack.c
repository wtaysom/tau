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

#include <style.h>

#define STSIZE	8

static u64	Stack[STSIZE];

u64 pop (void)
{
	unsigned	i;
	u64	x = Stack[0];

	for (i = 0; i < STSIZE-1; ++i)	{
		Stack[i] = Stack[i+1];
	}
	return x;
}

void push (u64 x)
{
	unsigned	i;

	for (i = STSIZE-1; i > 0; --i)	{
		Stack[i] = Stack[i-1];
	}
	Stack[0] = x;
}

void swap (void)
{
	u64	x = Stack[0];

	Stack[0] = Stack[1];
	Stack[1] = x;
}

void rotate (void)
{
	unsigned	i;
	u64	x = Stack[0];

	for (i = 0; i < STSIZE-1; ++i)	{
		Stack[i] = Stack[i+1];
	}
	Stack[STSIZE-1] = x;
}

void dup (void)
{
	push(Stack[0]);
}

u64 ith (unsigned i)
{
	if (i >= STSIZE)	return 0;
	return Stack[i];
}
