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
#include <assert.h>
#include "q.h"
#include "ants.h"
#include "lockEvents.h"

int main (int argc, char *argv[])
{
	LockAnt_s	a;
	Ant_s		from;

	LockTypeAnt.type.init( &a.ant, NULL);
	TypeAnt.type.init( &from, NULL);

	from.event = LOCK_SHARE;
	event( &a.ant, &from);
	from.event = LOCK_UNLOCK;
	event( &a.ant, &from);
	return 0;
}
