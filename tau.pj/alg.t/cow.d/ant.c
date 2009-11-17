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

#include <style.h>
#include <eprintf.h>
#include <debug.h>
#include <q.h>

#include "ant.h"

void init_ant (ant_s *ant, ant_caste_s *caste)
{
	zero(*ant);
	init_dq( &ant->a_ants);
	ant->a_caste = caste;
	ant->a_state = ANT_CLEAN;
}

void clear_queen (ant_s *ant)
{
	if (ant->a_queen) {
		ant->a_queen = NULL;
		rmv_dq( &ant->a_siblings);
	}
}

void queen_ant (ant_s *queen, ant_s *ant)
{
	if (ant->a_queen) {
		if (ant->a_queen == queen) return;
		fatal("ant already dependent on different queen");
		return;
	}
	assert( !is_qmember(ant->a_siblings));
	enq_dq( &queen->a_ants, ant, a_siblings);
	ant->a_queen = queen;
}

void flush_ant (ant_s *queen)
{
	ant_s	*ant;
FN;
	if (queen->a_state != ANT_DIRTY) return;
	queen->a_state = ANT_FLUSHING;
	for (;;) {
		deq_dq( &queen->a_ants, ant, a_siblings);
		if (!ant) break;
		flush_ant(ant);
	}
	if (queen->a_caste) queen->a_caste->ant_flush(queen);
	clear_queen(queen);
	queen->a_state = ANT_CLEAN;
}

void update_ant (ant_s *ant, void *user_data)
{
	if (!ant) return;
	if (!ant->a_caste) return;
	ant->a_caste->ant_update(ant, user_data);
}

void dirty_ant (ant_s *ant)
{
	ant->a_state = ANT_DIRTY;
}


