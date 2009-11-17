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
#include <tau/switchboard.h>

#include <eprintf.h>
#include <debug.h>
#include <cmd.h>

#include <cmd_hdr.h>

int failure (char *err, int rc)
{
	eprintf("FAILURE: %s rc=%d", err, rc);
	return rc;
}

static int testp (int argc, char *argv[])
{
	printf("Test\n");
	return 0;
}

static void init_net (void)
{
	if (init_msg_tau(getprogname())) {
		exit(2);
	}
	sw_register(getprogname());
}

int main (int argc, char *argv[])
{
	debugenv();
	setprogname(argv[0]);
	init_shell(NULL);

	init_net();

	init_sage();
	init_dir();
	init_test();

	CMD(test, "# test function");
	return shell();
}
