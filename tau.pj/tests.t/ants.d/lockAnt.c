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
#include "ants.h"
#include "lockEvents.h"

/*
 * Lock States
 */
enum { IDLE, SHARED, WAIT, EXCLUSIVE };

/*
 * Type Data
 */
extern State_s BufStates[][LOCK_NUM_EVENTS];

/*
 * Actions
 */

Ant_s *IdleShare (LockAnt_s *self, Ant_s *from)
{
	++self->count;
	self->ant.state = BufStates[SHARED];
	return NULL;
}

Ant_s *IdleExclusive (LockAnt_s *self, Ant_s *from)
{
	self->ant.state = BufStates[EXCLUSIVE];
	return NULL;
}

Ant_s *SharedShare (LockAnt_s *self, Ant_s *from)
{
	++self->count;
	return NULL;
}

Ant_s *SharedExclusive (LockAnt_s *self, Ant_s *from)
{
	Que( &self->ant, from);
	self->ant.state = BufStates[WAIT];
	return NULL;
}

Ant_s *SharedUnlock (LockAnt_s *self, Ant_s *from)
{
	--self->count;
	if (self->count == 0)
	{
		self->ant.state = BufStates[IDLE];
	}
	return NULL;
}

Ant_s *WaitUnlock (LockAnt_s *self, Ant_s *from)
{
	--self->count;
	if (self->count == 0)
	{
		self->ant.state = BufStates[IDLE];
		ready( &self->ant);	// Must be last
	}
	return NULL;
}

Ant_s *ExclusiveUnlock (LockAnt_s *self, Ant_s *from)
{
	self->ant.state = BufStates[IDLE];
	ready( &self->ant);	// Must be last
	return NULL;
}

/*
 * Terminal State
 */

/*
 * State Table
 */
#define STATE(_x_)	{(antfunc_t)_x_}
State_s BufStates[][LOCK_NUM_EVENTS] = {
	// IDLE
	{
		STATE(IdleShare),	// SHARE
		STATE(IdleExclusive),	// EXCLUSIVE
		STATE(Illegal)		// UNLOCK
	},
	// SHARED
	{
		STATE(SharedShare),	// SHARE
		STATE(SharedExclusive),	// EXCLUSIVE
		STATE(SharedUnlock)	// UNLOCK
	},
	// WAIT
	{
		STATE(Que),		// SHARE
		STATE(Que),		// EXCLUSIVE
		STATE(WaitUnlock)	// UNLOCK
	},
	// EXCLUSIVE
	{
		STATE(Que),		// SHARE
		STATE(Que),		// EXCLUSIVE
		STATE(ExclusiveUnlock)	// UNLOCK
	}
};

/*
 * Initial State
 */

Ant_s *initLock (LockAnt_s *self)
{
	LockTypeAnt.type.ant.super->init( &self->ant, NULL);
	self->ant.super = &LockTypeAnt.type;
	self->ant.state = BufStates[IDLE];
	self->count = 0;
	return &self->ant;
}


/*
 * Type Defintion
 */
LockTypeAnt_s	LockTypeAnt =
{
	{
		{ &TypeAnt.type },
		"lock", NULL, (antfunc_t)initLock, NULL
	}
};

