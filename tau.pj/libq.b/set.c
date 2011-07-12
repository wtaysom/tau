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

#include <stdio.h>

#include <style.h>
#include <set.h>
#include <eprintf.h>
#include <mylib.h>

enum { INIT_SET_SIZE = 16 };

void init_set (struct set_s *s)
{
	s->st_start = emalloc(INIT_SET_SIZE * sizeof(addr));
	s->st_last  = &s->st_start[INIT_SET_SIZE];
	s->st_next  = s->st_start;
}

void reinit_set (set_s *s)
{
	s->st_next  = s->st_start;
}

void add_to_set (set_s *s, addr x)
{
	int	size;
	int	newsize;
	addr	*new;

	if (s->st_next == s->st_last) {
		size = s->st_last - s->st_start;
		newsize = size * 2;
		new = erealloc(s->st_start, newsize * sizeof(addr));
		s->st_last = &new[newsize];
		s->st_next = &new[size];
		s->st_start = new;
	}
	*s->st_next++ = x;
}

bool remove_from_set (set_s *s, addr x)
{
	addr	*p;

	for (p = s->st_start; p != s->st_next; p++) {
		if (*p == x) {
			--s->st_next;
			*p = *s->st_next;
			return TRUE;
		}
	}
	return FALSE;
}

bool is_in_set (set_s *s, addr x)
{
	addr	*p;

	for (p = s->st_start; p != s->st_next; p++) {
		if (*p == x) {
			return TRUE;
		}
	}
	return FALSE;
}

bool rand_pick_from_set (set_s *s, addr *x)
{
	int	size;
	int	i;

	size = s->st_next - s->st_start;
	if (!size) return FALSE;
	i = urand(size);
	*x = s->st_start[i];
	return TRUE;
}

void pr_set (set_s *s)
{
	addr	*p;

	printf("set %p: start=%p next=%p last=%p\n\t:",
		s, s->st_start, s->st_next, s->st_last);
	for (p = s->st_start; p != s->st_next; p++) {
		printf(" %lu", *p);
	}
	printf("\n");
}
