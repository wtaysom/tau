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

#ifndef _ANT_H_
#define _ANT_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _Q_H_
#include <q.h>
#endif

typedef enum Ant_state_t {
	ANT_FREE = 0,
	ANT_CLEAN = 1,
	ANT_DIRTY = 2,
	ANT_FLUSHING = 3 } Ant_state_t;

typedef struct ant_s		ant_s;
typedef struct ant_caste_s	ant_caste_s;

struct ant_s {
	ant_caste_s	*a_caste;
	ant_s		*a_queen;
	dqlink_t	a_siblings;
	d_q		a_ants;
	u8		a_state;
};

struct ant_caste_s {
	const char	*ant_name;
	void		(*ant_flush)(ant_s *ant);
	void		(*ant_update)(ant_s *ant, void *user);
};

void init_ant    (ant_s *ant, ant_caste_s *caste);
void clear_queen (ant_s *ant);
void queen_ant   (ant_s *queen, ant_s *ant);
void flush_ant   (ant_s *ant);
void update_ant  (ant_s *ant, void *user_data);

#endif
