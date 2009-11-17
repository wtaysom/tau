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

#include <debug.h>
#include <eprintf.h>
#include <q.h>

#include <tau/msg.h>
#include <tau/tag.h>

#include <sw.h>

#define err_log	eprintf

int init (char *net_dev)
{
	int		rc;

FN;
	rc = init_net(net_dev);
	if (rc) return rc;

	rc = init_client();
	if (rc) return rc;

	return rc;
}

int main (int argc, char *argv[])
{
	int	rc;
	char	*dev = "eth0:not_right";

	debugenv();
FN;
	setprogname(argv[0]);
	if (argc > 1) {
		dev = argv[1];
	}

	rc = init_msg_tau(getprogname());
	if (rc) {
		eprintf("init_msg failed %d", rc);
	}

	rc = init(dev);
	if (rc) return rc;

	tag_loop();

	return 0;
}

