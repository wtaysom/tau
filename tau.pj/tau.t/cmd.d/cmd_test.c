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
#include <string.h>

#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/switchboard.h>

#include <eprintf.h>
#include <cmd.h>

#include <dirfs.h>

#include <cmd_hdr.h>

static int sendp (int argc, char *argv[])
{
	msg_s	msg;
	ki_t	ki;
	int	rc;

	ki = make_gate(0, REPLY | PASS_ANY);
	if (!ki) return -1;

	rc = send_tau(ki, &msg);
	if (rc) return failure("send tst: send", rc);

	return 0;
}


int errfail (char *err, int expected, int rc)
{
	eprintf("FAILURE: %s expected %d but got %d", err, expected, rc);
	return expected;
}

static int errp (int argc, char *argv[])
{
	msg_s		msg;
	int		rc;

	rc = send_tau(0, &msg);
	if (rc != EBADKEY) return errfail("send tst: send", EBADKEY, rc);

	return 0;
}

void init_test (void)
{
	CMD(send,	"# send test");
	CMD(err,	"# error tests");
}
