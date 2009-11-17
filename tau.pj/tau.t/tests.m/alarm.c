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
#include <tau/alarm.h>
#include <tau/msg_util.h>

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
	printf("alarm:%s %d> ", s, cnt);
	if (Ignore_prompt) {
		printf("\n");
	} else {
		getchar();
	}
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
	ki_t	alarm_key;
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
	sw_register("alarm_test");

prompt("create_gate");
	key = make_gate(0, REQUEST);

prompt("cyclic_alarm");
	rc = cyclic_alarm(key, 3000, &alarm_key);
	if (rc) failure("cyclic_alarm", rc);

	for (;;) {
prompt("receive");
		rc = receive_tau( &msg);
		if (rc) failure("receive", rc);

		printf("Timer popped\n");
	}
}
