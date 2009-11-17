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

#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/switchboard.h>
#include <tau/tag.h>

#include <eprintf.h>
#include <debug.h>

#include <sage_hdr.h>

void next_fm (void *msg);

typedef struct sage_s {
	struct type_s	*s_type;
} sage_s;

sage_s	Sage;

int failure (char *err, int rc)
{
	eprintf("FAILURE: %s rc=%d", err, rc);
	return rc;
}

static void init_net (void)
{
	ki_t	key;
	int	rc;
FN;
	if (init_msg_tau(getprogname())) {
		exit(2);
	}
	sw_register(getprogname());

	Sage.s_type = make_type("sage", NULL, next_fm, NULL);
	key = make_gate( &Sage, REQUEST | PASS_REPLY);

	rc = sw_post("sage", key);
	if (rc) failure("sw_post", rc);
}

int main (int argc, char *argv[])
{
	debugenv();
	setprogname(argv[0]);
FN;
	init_net();

	init_fs();
	init_tau();

	tag_loop();

	return 0;
}
