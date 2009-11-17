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
 * Each ant has two files.  An *Events.h which has all the
 * events and message formats and an a *Ant.c that has the
 * state tables and functions.
 *
 * An object, can be composed of multiple ants.
 */
#include <stdio.h>
#include <string.h>
#include <q.h>
#include <ants.h>

bool	Debugging = TRUE;

void debug (Ant_s *to, Ant_s *from)
{
	printf("from %s:%p to %s:%p event=%d\n",
		from->super->name,	from,
		to->super->name,	to,
		from->event);
}

Ant_s *Ignore (Ant_s *self, Ant_s *from)
{
	if (Debugging) printf("Ignore\n");
	return NULL;
}

Ant_s *Que (Ant_s *self, Ant_s *from)
{
	if (Debugging) printf("Que\n");
	enq_cir( &self->waitq, from, waiting);
	return NULL;
}

Ant_s *Illegal (Ant_s *self, Ant_s *from)
{
	printf("Illegal event\n");
	return NULL;
}

	/*
	 * Two ways to send a message:
	 *	1.  Call the event function.
	 *	2.  Return a pointer to the ant to send
	 *		the next event to.
	 */
void event (Ant_s *to, Ant_s *from)
{
	Ant_s	*next;

	for (;;)
	{
		if (is_qmember(to->waiting))
		{		/*
				 * If the destination is already waiting,
				 * just queue it up.
				 */
			enq_cir( &to->waitq, from, waiting);
			return;
		}
		if (Debugging)
		{
			debug(to, from);
		}
		next = to->state[from->event].action(to, from);
		if (!next)
		{		/*
				 * No more messages to process.
				 */
			return;
		}
		from = to;
		to = next;
	}
}

	/*
	 * Called from with in an ant routine
	 * when it needs to ready everything
	 * that has been waiting.
	 *
	 * Must be called last in action routines.
	 *
	 * May need to look at ways to just process
	 * as much of the queue as we can until things
	 * begin to queue.
	 */
void ready (Ant_s *self)
{
	cir_q	readyq;
	Ant_s	*from;

	init_cir( &readyq);
	if (is_empty_cir( &self->waitq))
	{
		return;
	}
	readyq = self->waitq;
	init_cir( &self->waitq);

	for (;;)
	{
		deq_cir( &readyq, from, waiting);
		if (!from)
		{
			return;
		}
		event(self, from);
	}
}

Ant_s *initAnt (Ant_s *self, Ant_s *ignore)
{
	bzero(self, sizeof(Ant_s));
	self->super = &TypeAnt.type;
	return self;
}

TypeAnt_s	TypeAnt = { {{0}, "ant", NULL, initAnt, NULL} };
