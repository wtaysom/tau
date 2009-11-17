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

#include <tau/msg.h>

typedef struct tstmsg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u	b;

		struct {
			char	tst_data;
		};
	};
} tstmsg_s;

int failure (char *err, int rc)
{
	fprintf(stderr, "FAILURE: %s rc=%d\n", err, rc);
	exit(2);
	return 0;
}

void prompt (void)
{
	static int	cnt = 0;

	++cnt;
	printf("%d> ", cnt);
	getchar();
}

int main (int argc, char *argv[])
{
	tstmsg_s	msg;
	unsigned	key;
	int		rc;

prompt();
	if (init_msg_tau(argv[0])) {
		exit(2);
	}
prompt();
	msg.q.q_tag     = &msg;
	msg.q.q_type    = REQUEST;
	rc = create_gate_tau( &msg);
prompt();
	if (rc) failure("create_gate", rc);
	key = msg.q.q_passed_key;

	msg.tst_data = 13;
	rc = send_tau(key, &msg);
prompt();
	if (rc) failure("send", rc);

	rc = receive_tau( &msg);
prompt();
	if (rc) failure("receive", rc);

	if (msg.q.q_tag != &msg) failure("tag wrong", 0);
	if (msg.tst_data != 13) failure("msg body", 0);

	rc = destroy_key_tau(key);
prompt();
	if (rc) failure("destroykey", rc);

	return 0;
}
