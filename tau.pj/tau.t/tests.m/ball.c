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

#include <style.h>
#include <eprintf.h>
#include <timer.h>

#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>

/* Juggling of colored balls - pass them around */

int failure (char *err, int rc)
{
	eprintf("FAILURE: %s rc=%d\n", err, rc);
	return rc;
}

bool Prompt = FALSE;
void prompt_on (void)
{
	Prompt = TRUE;
}

void prompt (char *s)
{
	static int	cnt = 0;

	if (!Prompt) return;
	++cnt;
	printf("ping:%s %d> ", s, cnt);
	getchar();
}

void usage (void)
{
	fprintf(stderr, "usage: %s [-?i] send_color receive_color [iterations]\n"
		"\t-?  this message\n"
		"\t-p  turn on prompt\n",
 		getprogname());
	exit(1);
}

int main (int argc, char *argv[])
{
	msg_s	msg;
	ki_t	key;
	int	c;
	int	rc;
	char	*send_color;
	char	*recv_color;
	unint	i;
	unint	n = 1000;
	u64	start = 0;
	u64	finish;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "p?")) != -1) {
		switch (c) {
		case 'p':
			prompt_on();
			break;
		default:
			usage();
			break;
		}
	}
	if (argc < optind+2) usage();

	send_color = argv[optind];
	recv_color = argv[optind+1];

	if (argc > optind+2) {
		n = strtoll(argv[optind+2], NULL, 0);
	}

prompt("start");
	if (init_msg_tau(send_color)) {
		exit(2);
	}
prompt("sw_register");
	sw_register(send_color);

prompt("create_gate_tau");
	key = make_gate(0, REQUEST);

prompt("sw_post");
	rc = sw_post(send_color, key);
	if (rc) failure("sw_post", rc);

prompt("sw_any");
	rc = sw_any(recv_color, &key);
	if (rc) failure("sw_any", rc);

	for (i = 0; i < n; i++) {
		if (i == 2) start = nsecs();
prompt("send_tau");
		rc = send_tau(key, &msg);
		if (rc) failure("send_tau", rc);

prompt("receive_tau");
		rc = receive_tau( &msg);
		if (rc) failure("receive", rc);
	}
	finish = nsecs();
	printf("%s %g nsecs per send/receive\n", send_color,
		((double)(finish - start))/(n-2));
	return 0;
}
