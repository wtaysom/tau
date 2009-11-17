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
#include <stdlib.h>

#include <style.h>
#include <timer.h>

slot_s *max_slot (slot_s *slot, tick_t *avg)
{
	slot_s	*end = &slot[NUM_SLOTS];
	slot_s	*max = slot;
	tick_t	sum = slot->s_avg;

	for (++slot; slot != end; slot++) {
		sum += slot->s_avg;
		if (slot->s_delta > max->s_delta) {
			max = slot;
		}
	}
	*avg = (sum + NUM_SLOTS/2) / NUM_SLOTS;
	return max;
}

void cascade (cascade_s w, tick_t delta)
{
	tick_t	avg;
	slot_s	*s;
	slot_s	*next;

	s = &w->wh_slot[w->wh_next];
	s->s_delta = delta;
	s->s_avg   = delta;

	for (;;) {
		if (++w->wh_next != NUM_SLOTS) {
			break;
		}
		w->wh_next = 0;
		s = max_slot(w->wh_slot, &avg);

		++w;
		next = &w->wh_slot[w->wh_next];
		next->s_delta = s->s_delta;
		next->s_avg   = avg;
	}
}

static int pr_slot (slot_s *s)
{
	if (!s->s_delta) return FALSE;
	printf("%'15lld %'12lld\n", s->s_delta, s->s_avg);
	return TRUE;
}

static int pr_wheel (wheel_s *w)
{
	slot_s	*s;
	int	more = TRUE;

	for (s = &w->wh_slot[w->wh_next]; s != &w->wh_slot[NUM_SLOTS]; s++) {
		if (!pr_slot(s)) {
			more = FALSE;
			break;
		}
	}
	for (s = w->wh_slot; s != &w->wh_slot[w->wh_next]; s++) {
		if (!pr_slot(s)) return FALSE;
	}
	printf("\n");
	return more;
}

void pr_cascade (const char *label, cascade_s wheel)
{
	wheel_s		*w;

	printf("%s\n", label);
	for (w = wheel; w < &wheel[NUM_WHEELS]; w++) {
		if (!pr_wheel(w)) break;
	}
}

#if 0
int main (int argc, char *argvp[])
{
	tick_t	i;
	tick_t	start;
	tick_t	delta;

//	printf("%lld\n", cputicks());
//	printf("%lld\n", cputicks());

	start = cputicks();
	for (i = 0; i < 400017LL; i++) {
		delta = cputicks() - start;
		start = cputicks();
		cascade(delta);
	}
	pr_cascade();
	return 0;
}
#endif
