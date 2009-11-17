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

#ifndef _SET_H_
#define _SET_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct set_s {
	addr	*st_start;
	addr	*st_next;
	addr	*st_last;
} set_s;

void init_set        (set_s *s);
void reinit_set      (set_s *s);
void add_to_set      (set_s *s, addr x);
bool remove_from_set (set_s *s, addr x);
bool is_in_set       (set_s *s, addr x);

bool rand_pick_from_set (set_s *s, addr *x);
void pr_set (set_s *s);

#endif
