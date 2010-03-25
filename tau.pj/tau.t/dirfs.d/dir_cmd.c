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

#include <dirfs.h>

#include <dir_hdr.h>

typedef struct cmd_type_s {
	type_s		tc_tag;
	method_f	tc_crdir;
	method_f	tc_prtree;
	method_f	tc_prbackup;
	method_f	tc_readdir;
} cmd_type_s;

typedef struct cmd_s {
	type_s		*cmd_type;
} cmd_s;

void destroy_cmd  (void *m);
void crdir_cmd    (void *m);
void prtree_cmd   (void *m);
void prbackup_cmd (void *m);
void readdir_cmd  (void *m);

cmd_type_s	Cmd_type = { { "Cmd", DIR_OPS, destroy_cmd },
				crdir_cmd,
				prtree_cmd,
				prbackup_cmd,
				readdir_cmd };

cmd_s		Cmd = { &Cmd_type.tc_tag };


typedef struct read_type_s {
	type_s		tr_tag;
	method_f	tr_done;
} read_type_s;

typedef struct read_s {
	type_s		*read_type;
} read_s;

void destroy_read (void *m);
void done_read (void *m);

read_type_s	Read_type = { { "Read", DIR_OPS, destroy_read },	// WRONG
				done_read };

read_s		Read = { &Read_type.tr_tag };


void destroy_read (void *m)
{
FN;
	PRs("destroy_read called");
}

void done_read (void *m)
{
FN;
	PRs("done_read called");
}

void destroy_cmd (void *m)	// Should never get called -- make 0
{
FN;
	eprintf("destroy cmd called");
}

void crdir_cmd (void *m)
{
	fs_msg_s	*msg = m;
	unsigned	reply_key;
	int		rc;

	reply_key = msg->q.q_passed_key;
//	printf("Path=%s\n", &msg->b);
	cr_dir(msg->cr_path);
	rc = send_tau(reply_key, msg);
	if (rc) failure("cpdir_cmd: send", rc);
}

void prtree_cmd (void *m)
{
	msg_s		*msg = m;
	unsigned	reply_key;
	int		rc;

	reply_key = msg->q.q_passed_key;
	pr_tree();
	rc = send_tau(reply_key, msg);
	if (rc) failure("prtree_cmd: send", rc);
}

void prbackup_cmd (void *m)
{
	msg_s		*msg = m;
	unsigned	reply_key;
	int		rc;

	reply_key = msg->q.q_passed_key;
	pr_backup();
	rc = send_tau(reply_key, msg);
	if (rc) failure("prbackup_cmd: send", rc);
}

void readdir_cmd (void *msg)
{
	fs_msg_s	*m = msg;
	unsigned	reply_key;
	unsigned	data_key;
	dir_s		*dir;
	u64		id;
	int		rc;

	reply_key = m->q.q_passed_key;
	id = m->rd_dir_num;

	if (is_local(id)) {
		dir = read_dir(m->rd_dir_num);
		if (!dir) {
			failure("readdir_cmd: Couln't find", id);
			return;
		}
		data_key = make_datagate( &Read, REPLY | READ_DATA,
				dir, sizeof *dir);
		if (!data_key) {
			failure("readdir_cmd: make_datagate", -1);
			return;
		}
		m->q.q_passed_key = data_key;
		rc = send_key_tau(reply_key, m);
		if (rc) {
			failure("readdir_cmd: send_key_tau", rc);
			return;
		}
	} else {
		m->m_method = PEER_READDIR;
		rc = send_key_tau(remote(id), m);
		if (rc) {
			failure("readdir_cmd: send", rc);
			return;
		}
	}
}

int init_cmd (void)
{
	ki_t	key;
	int	rc;

	key = make_gate( &Cmd, REQUEST | PASS_REPLY);
	rc = sw_post("dir_cmd", key);
	if (rc) failure("sw_post", rc);
	return rc;
}
