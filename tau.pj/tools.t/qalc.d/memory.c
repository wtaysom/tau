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

#include <qalc.h>

extern uquad	Memory[MAX_MEM];

void initMemory (void)
{
	bzero(Memory, sizeof(Memory));
}

void store (void)
{
	index_t	n;

	n = pop();
	if (n >= MAX_MEM)	return;

	dup();

	Memory[n] = pop();
}

void retrieve (void)
{
	index_t	n;

	n = pop();
	if (n >= MAX_MEM)	return;

	push(Memory[n]);
}
