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
#include <tau/msg.h>
#include <tau/tag.h>

static int	Prompt = FALSE;

static void prompt (void)
{
	if (Prompt) {
		printf("%s-> ", getprogname());
		getchar();
	}
}

static void cleanup_msg (msg_s *msg)
{
	if (msg->q.q_passed_key) {
		destroy_key_tau(msg->q.q_passed_key);
	}
}

static void tag_method (void *m)
{
	msg_s		*msg  = m;
	object_s	*obj  = msg->q.q_tag;
	type_s		*type = obj->o_type;

FN;
	if (msg->m_method < type->ty_num_methods) {
		type->ty_method[msg->m_method](msg);
	} else {
		cleanup_msg(msg);
		weprintf("tag_method:method number out of range %d\n",
			msg->m_method);
	}
}

static void tag_destroy (void *m)
{
	msg_s		*msg  = m;
	object_s	*obj  = msg->q.q_tag;
	type_s		*type = obj->o_type;

FN;
	type->ty_destroy(msg);
}

void tag_loop (void)
{
	msg_s	msg;
	int	rc;

FN;
	for (;;) {
		prompt();
		rc = receive_tau( &msg);
		if (rc) {
			if (rc == DESTROYED) {
				tag_destroy( &msg);
			} else {
				weprintf("tag_loop:receive rc=%d", rc);
			}
		} else {
			tag_method( &msg);
		}
	}
}
