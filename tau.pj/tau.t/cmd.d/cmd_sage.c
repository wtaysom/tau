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

#include <style.h>

#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/switchboard.h>
#include <sage.h>
#include <cmd.h>

#include <cmd_hdr.h>

bool	Did_mkfs = FALSE;

static unsigned	Sage_key;

static int mkfsp (int argc, char *argv[])
{
	msg_s	msg;
	int	rc;

	msg.m_method = SAGE_MKFS;
	rc = call_tau(Sage_key, &msg);
	if (rc) return failure("mkfs:call_tau", rc);
	Did_mkfs = TRUE;
	return rc;
}

void init_sage (void)
{
	int		rc;

	rc = sw_any("sage", &Sage_key);
	if (rc) failure("sw_any sage", rc);

	CMD(mkfs,	"# make file system");
}
