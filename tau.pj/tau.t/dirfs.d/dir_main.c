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
#include <tau/switchboard.h>
#include <tau/tag.h>
#include <tau/msg_util.h>

#include <eprintf.h>
#include <debug.h>

#include "dir_hdr.h"

int failure (char *err, int rc)
{
	eprintf("FAILURE: %s rc=%d", err, rc);
	return rc;
}

static int	Prompt = FALSE;

void prompt (char *s)
{
	static int	cnt = 0;

	++cnt;
	if (Prompt) {
		printf("dir:%s %d> ", s, cnt);
		getchar();
	}
}

int init_net (void)
{
	int		rc;

	rc = init_msg_tau(getprogname());
	if (rc) {
		eprintf("init_msg failed %d", rc);
	}
	sw_register(getprogname());

	rc = init_cmd();
	if (rc) return failure("init_cmd", rc);

	rc = init_hello();
	if (rc) return failure("init_hello", rc);

	rc = init_sage();
	if (rc) return failure("init_sage", rc);

	return rc;
}

int main (int argc, char *argv[])
{
	int	rc;

	debugenv();
FN;
	setprogname(argv[0]);

	rc = init_fs();
	if (rc) return rc;

	rc = init_net();
	if (rc) return rc;

	tag_loop();

	return 0;
}
