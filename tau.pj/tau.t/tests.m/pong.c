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

#include <tau/msg.h>
#include <tau/switchboard.h>

/* PONG */

int failure (char *err, int rc)
{
	eprintf("FAILURE: %s rc=%d\n", err, rc);
	return 0;
}

int Ignore_prompt = FALSE;
void ignore_prompt (void)
{
	Ignore_prompt = TRUE;
}

void prompt (char *s)
{
	static int	cnt = 0;

	++cnt;
	printf("pong:%s %d> ", s, cnt);
	if (Ignore_prompt) {
		printf("\n");
	} else {
		getchar();
	}
}

ki_t make_gate (
	void		*tag,
	unsigned	type)
{
	msg_s	msg;
	int	rc;

	msg.q.q_tag     = tag;
	msg.q.q_type    = type;
	rc = create_gate_tau( &msg);
	if (rc) {
		failure("make_gate: create_gate failed", rc);
	}
	return msg.q.q_passed_key;
}

void usage (void)
{
	fprintf(stderr, "usage: %s [-?i]\n"
		"\t-?  this message\n"
		"\t-i  ignore prompt\n",
 		getprogname());
	exit(1);
}

int main (int argc, char *argv[])
{
	msg_s	msg;
	ki_t	key;
	int	c;
	int	rc;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "i")) != -1) {
		switch (c) {
		case 'i':
			ignore_prompt();
			break;
		default:
			usage();
			break;
		}
	}

prompt("start");
	if (init_msg_tau(getprogname())) {
		exit(2);
	}
prompt("sw_register");
	sw_register(argv[0]);

prompt("create_gate");
	key = make_gate(0, REQUEST | PASS_REPLY);

prompt("sw_post");
	rc = sw_post("pong", key);
	if (rc) failure("sw_post", rc);

	for (;;) {
prompt("receive");
		rc = receive_tau( &msg);
		if (rc) failure("receive", rc);

prompt("send");
		rc = send_tau(msg.q.q_passed_key, &msg);
		if (rc) failure("send", rc);
	}
}
