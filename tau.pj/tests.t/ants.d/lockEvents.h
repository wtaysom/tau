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
/*
 * Lock events
 */
typedef enum LockEvents_t {
	LOCK_SHARE,
	LOCK_EXCLUSIVE,
	LOCK_UNLOCK,
	LOCK_NUM_EVENTS
} LockEvents_t;

/*
 * Lock messages -- no parameters, no messages
 */


/*
 * Lock ant structure
 */
typedef struct LockAnt_s
{
	Ant_s	ant;
	/* Messages */
	/* Local Variables */	// Need away of making this private
	unint	count;
} LockAnt_s;

/*
 * Type Defintion
 */
typedef struct LockTypeAnt_s
{
	Type_s	type;
} LockTypeAnt_s;

extern LockTypeAnt_s	LockTypeAnt;


