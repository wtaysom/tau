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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <eprintf.h>
#include <debug.h>

#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/alarm.h>
#include <tau/msg_util.h>
#include <tau/net.h>

enum { ALARM_INTERVAL = 2000 };
enum { MAX_WATCHDOGS = 16 };
enum { INIT_QUERY = 0};
enum { FINISH_INIT = 0, I_AM_A_LIVE };
enum { ARE_YOU_A_LIVE = 0 };

typedef struct query_s {
	type_s	*q_type;
	key_t	q_key;
	u64	q_node;
	u64	q_sent;
	u64	q_recv;
} query_s;

typedef struct respond_s {
	type_s	*r_type;
	key_t	r_key;
} respond_s;

ki_t		Alarm_key;

type_s		*Query_type;
query_s		Query[MAX_NODES];
query_s		*NextQuery = Query;

type_s		*Respond_type;
respond_s	Respond[MAX_NODES];
respond_s	*NextRespond = Respond;

void alarm_died (void *msg)
{
	fatal("Cyclic alarm went away, die");
}

void query_died (void *msg)
{
	warn("query died");
}

void respond_died (void *msg)
{
	warn("respond died");
}

void supplied_watchdog (void *msg)
{
	msg_s		*m = msg;
	ki_t		reply = m->q.q_passed_key;
	query_s		*q;
	int		rc;
FN;
	if (NextQuery == &Query[MAX_WATCHDOGS]) {
		fatal("Out of watchdogs");
	}
	q = NextQuery++;
	q->q_type = Query_type;
	m->q.q_passed_key = make_gate(q, RESOURCE | PASS_OTHER);

	m->m_method = INIT_QUERY;
	rc = send_key_tau(reply, m);
	if (rc) fatal("send_key_tau %d", rc);
}

void init_query (void *msg)
{
	msg_s		*m = msg;
	ki_t		reply = m->q.q_passed_key;
	respond_s	*r;
	int		rc;
FN;
	if (NextRespond == &Respond[MAX_WATCHDOGS]) {
		fatal("Out of watchdogs");
	}
	r = NextRespond++;
	r->r_type = Respond_type;
	r->r_key  = reply;
	m->q.q_passed_key = make_gate(r, RESOURCE | PASS_OTHER);

	m->m_method = FINISH_INIT;
	rc = send_key_tau(reply, m);
	if (rc) fatal("send_key_tau %d", rc);
}

void finish_init (void *msg)
{
	msg_s		*m = msg;
	query_s		*q = m->q.q_tag;
	int		rc;
FN;
	q->q_key = m->q.q_passed_key;

	rc = stat_key_tau(q->q_key, m);
	if (rc) fatal("stat_key_tau %d", rc);
	q->q_node = m->sk_node_no;
}

void alarm_pop (void *msg)
{
	msg_s		*m = msg;
	query_s		*q;
	int		rc;

	printf("Timer popped!\n");

	for (q = Query; q < NextQuery; q++) {
		m->m_method = ARE_YOU_A_LIVE;
		if (q->q_sent - q->q_recv > 1) {
			warn("Missing a receive from %lld", q->q_node);
			node_died_tau(q->q_node);
			sleep(30);
		}
		rc = send_tau(q->q_key, msg);
		if (rc) fatal("send_tau %d", rc);
		++q->q_sent;
	}
}

void are_you_a_live (void *msg)
{
	msg_s		*m = msg;
	respond_s	*r = m->q.q_tag;
	int		rc;
FN;
	m->m_method = I_AM_A_LIVE;
	rc = send_tau(r->r_key, m);
	if (rc) fatal("send_tau %d", rc);
}

void i_am_a_live (void *msg)
{
	msg_s		*m = msg;
	query_s		*q = m->q.q_tag;
FN;
	++q->q_recv;
}

void switchboard (void)
{
	static type_s	*supplied;
	static type_s	*watchdogs;
	static type_s	*alarm;

	ki_t	key;
	int	rc;
FN;
	rc = sw_register(getprogname());
	if (rc) fatal("sw_register %d", rc);

	supplied = make_type("post watchdog", NULL, init_query, NULL);
	key = make_gate( &supplied, REQUEST | PASS_OTHER);
	rc = sw_post("watchdog", key);
	if (rc) fatal("sw_post watchdog %d", rc);

	watchdogs = make_type("request watchdogs", NULL, supplied_watchdog, NULL);
	key = make_gate( &watchdogs, REQUEST | PASS_OTHER);
	rc = sw_remote("watchdog", key);
	if (rc) fatal("sw_remote watchdog %d", rc);

	alarm = make_type("alarm", alarm_died, alarm_pop, NULL);
	key = make_gate( &alarm, RESOURCE);
	rc = cyclic_alarm(key, ALARM_INTERVAL, &Alarm_key);
	if (rc) fatal("cyclic_alarm %d", rc);

	Query_type   = make_type("query", query_died,
					finish_init, i_am_a_live, NULL);
	Respond_type = make_type("respond", respond_died,
					are_you_a_live, NULL);
}

void usage (void)
{
	fprintf(stderr, "usage: %s [-?]\n"
		"\t-?  this message\n",
		getprogname());
	exit(1);
}

int main (int argc, char *argv[])
{
	int	c;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "i")) != -1) {
		switch (c) {
		default:
			usage();
			break;
		}
	}

	if (init_msg_tau(getprogname())) {
		fatal("init_msg_tau");
	}
	switchboard();

	tag_loop();

	return 0;
}
