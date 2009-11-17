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
 * Each object has two files.  An objEvents.h which has all the
 * events and message formats and an a objAnt.c that has the state
 * tables and functions.
 */
#ifndef _ANTS_H_
#define _ANTS_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _Q_H_
#include <q.h>
#endif

typedef struct Ant_s	Ant_s;

typedef Ant_s	*(*antfunc_t)(Ant_s *to, Ant_s *from);

typedef struct State_s {
	antfunc_t	action;
} State_s;

struct Ant_s {
	struct Type_s	*super;
	State_s		*state;
	cir_q		waitq;
	qlink_t		waiting;
	unsigned	event;
	/* Message - a union of message types */
	/* Local Variables */
};

typedef struct Type_s {
	Ant_s	ant;
	/* Message - a union of message types */
	/* Local Variables */
	char	*name;
	/* Functions */
	antfunc_t	alloc;
	antfunc_t	init;
	antfunc_t	free;
} Type_s;

typedef struct TypeAnt_s {
	Type_s	type;
} TypeAnt_s;

/*
 * Globals
 */
extern TypeAnt_s	TypeAnt;

/*
 * Some default actions
 */
Ant_s *Ignore(Ant_s *self, Ant_s *from);
Ant_s *Que(Ant_s *self, Ant_s *from);
Ant_s *Illegal(Ant_s *self, Ant_s *from);

/*
 * Helper routines
 */
void ready(Ant_s *self);
void event (Ant_s *to, Ant_s *from);

#endif
