/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eprintf.h>
#include <q.h>
#include <style.h>

typedef enum state_t { NEW = 0, INITED, ADDED, DETACHED } state_t;

typedef struct Action_s Action_s;
typedef struct State_s State_s;

struct State_s {
	State_s		*next;
	cir_q		actions;
	u64		address;
	state_t		state;
};
	
struct Action_s {
	qlink_t	link;
	double	time;
	char	action[20];
	u64	address;
};

State_s *States = NULL;
bool Ignore = TRUE;

void error (char *msg, State_s *s, Action_s *a)
{
	printf("%s %d %10.10lg %s %llx\n",
		msg, s->state, a->time, a->action, a->address);
}

void ignore (char *msg, State_s *s, Action_s *a)
{
	if (!Ignore) printf("WARN: %s %d %10.10lg %s %llx\n",
		msg, s->state, a->time, a->action, a->address);
}


void do_transition (State_s *s, Action_s *a)
{
	if (strcmp(a->action, "init") == 0) {
		switch (s->state) {
		case NEW:					break;
		case INITED:	ignore("double init", s, a);	break;
		case ADDED:	error("init added", s, a);	break;
		case DETACHED:					break;
		default:	error("bad state", s, a);	break;
		}
		s->state = INITED;
	} else if (strcmp(a->action, "add") == 0) {
		switch (s->state) {
		case NEW:	ignore("not inited", s, a);	break;
		case INITED:					break;
		case ADDED:	error("double add", s, a);	break;
		case DETACHED:	ignore("not inited", s, a);	break;
		default:	error("bad state", s, a);	break;
		}
		s->state = ADDED;
	} else if (strcmp(a->action, "detach") == 0) {
		switch (s->state) {
		case NEW:	ignore("not inited", s, a);	break;
		case INITED:	error("not added", s, a);	break;
		case ADDED:					break;
		case DETACHED:	error("double detach", s, a);	break;
		default:	error("bad state", s, a);	break;
		}
		s->state = DETACHED;
	} else {
		if (!Ignore) printf("ignoring %s\n", a->action);
	}
}

State_s *find_state (u64 address)
{
	State_s	*s = States;

	while (s) {
		if (address == s->address) return s;
		s = s->next;
	}
	return NULL;
}

void add_state (State_s *s)
{
	s->next = States;
	States = s;
}

void add_action (Action_s *a)
{
	State_s	*s;

	s = find_state(a->address);
	if (!s) {
		s = ezalloc(sizeof(*s));
		s->address = a->address;
		add_state(s);
		init_cir( &s->actions);
	}
	enq_cir( &s->actions, a, link);
	do_transition(s, a);
}

int main (int argc, char *argv[])
{
	Action_s	*a;
	int	rc;

	for (;;) {
		a = ezalloc(sizeof(*a));
		rc = scanf("%lg%s%llx\n", &a->time, a->action, &a->address);
		if (rc != 3) break;
//		printf("%10.10lg %s %llx\n", a->time, a->action, a->address);
		add_action(a);
	}
	return 0;
}

