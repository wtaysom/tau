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

#include <style.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>

#include <data.h>

/* Reader */

int Ignore_prompt = FALSE;
void ignore_prompt (void)
{
	Ignore_prompt = TRUE;
}

void prompt (char *s)
{
	static int	cnt = 0;

	++cnt;
	printf("reader:%s %d> ", s, cnt);
	if (Ignore_prompt) {
		printf("\n");
	} else {
		getchar();
	}
}

int failure (char *err, int rc)
{
	weprintf("FAILURE: %s rc=%d\n", err, rc);
	prompt("failure:");
	return 0;
}

static int check_data (u8 *buf, unsigned length, unsigned seed)
{
	unsigned	i;
	u8		x;

	srandom(seed);

	for (i = 0; i < length; i++) {
		x = random();
		if (buf[i] != x) {
			weprintf("check failed at %d %x!=%x\n",
				i, buf[i], x);
			prompt("failure:");
			return FALSE;
		}
	}
	return TRUE;
}

void usage (void)
{
	fprintf(stderr, "usage: %s [-?i]\n"
		"\t-?  this message\n"
		"\t-i  ignore prompt\n",
 		getprogname());
	exit(1);
}

u8	Buf[2001];

int main (int argc, char *argv[])
{
	data_msg_s	msg;
	ki_t		key;
	ki_t		data_key;
	unsigned	i;
	int		c;
	int		rc;

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
	key = make_gate(0, REQUEST | PASS_OTHER);

prompt("sw_request");
	rc = sw_request("data", key);
	if (rc) failure("sw_request", rc);

prompt("receive_tau");
	rc = receive_tau( &msg);
	if (rc) failure("receive_tau", rc);

	data_key = msg.q.q_passed_key;

	for (i = 0;; i++) {
prompt("read_data");
		msg.m_method = TST_READ_DATA;
		msg.dt_num_bytes = sizeof Buf;
		msg.dt_seed      = i;
		rc = call_tau(data_key, &msg);
		if (rc) failure("call_tau", rc);

		rc = read_data_tau(msg.q.q_passed_key, sizeof Buf, Buf, 0);
		if (rc) failure("read_data_tau", rc);

		check_data(Buf, sizeof Buf, msg.dt_seed);

//		printf("%d.\n", i);
//		prmem("Buf", Buf, sizeof Buf);

		rc = destroy_key_tau(msg.q.q_passed_key);
		if (rc) failure("destroy_key_tau", rc);
	}
}
