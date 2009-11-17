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

#include <tau/tag.h>
#include <tau/switchboard.h>
#include <tau/msg_util.h>

#include <debug.h>
#include <eprintf.h>

#include <dir_hdr.h>

typedef struct sage_type_s {
	type_s		tsg_tag;
	method_f	tsg_mkfs;
} sage_type_s;

typedef struct sage_s {
	type_s		*sg_type;
} sage_s;

static void mkfs_cmd (void *m);

sage_type_s	Sage_type = { {FS_OPS, 0 },
				mkfs_cmd };

sage_s		Sage = { &Sage_type.tsg_tag };

static void  mkfs_cmd (void *msg)
{
	fs_msg_s	*m = msg;
	unsigned	reply_key;

	reply_key = m->q.q_passed_key;	// Doesn't have a reply key yet

	mkfs_dir(m->mk_total_shares,
		m->mk_my_shares,
		m->mk_my_backup);
}

int init_sage (void)
{
	ki_t	key;
	int	rc;
FN;
	key = make_gate( &Sage, REQUEST | PASS_OTHER);
	rc = sw_post("dir_sage", key);
	if (rc) failure("sw_post", rc);

	return rc;
}
