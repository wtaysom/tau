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

#include <string.h>

#include <eprintf.h>
#include <debug.h>

#include <tau/switchboard.h>
#include <tau/tag.h>
#include <tau/msg_util.h>

#include <sw.h>

typedef struct client_type_s {
	type_s		tc_tag;
	method_f	tc_ops[SW_OPS];
} client_type_s;

struct client_s {
	type_s	*c_type;
	name_s	*c_name;
};

static void client_destroy (void *m);
static void post_key       (void *m);
static void get_keys       (void *m);
#if 0
static void all_keys       (void *m);
static void local_key      (void *m);
static void remote_keys    (void *m);
static void any_key        (void *m);
#endif
static void next_key       (void *m);

client_type_s	Client_type = {
			{ SW_OPS, client_destroy },{
			post_key,
			get_keys,
			next_key}};

static client_s	Client[MAX_CLIENTS];
static client_s	*NextClient = Client;

client_s *client_new (name_s *name)
{
	client_s	*client;
FN;
	if (NextClient == &Client[MAX_CLIENTS]) {
		eprintf("Too many clients %s", name);
		return NULL;
	}
	client = NextClient++;
	client->c_type = &Client_type.tc_tag;
	client->c_name  = name_find(name);

	return client;
}

static void client_delete (client_s *client)
{
FN;
	request_delete(client);
	key_delete(client);
	zero(*client);
}

static void client_destroy (void *m)
{
	sw_msg_s	*msg	= m;
	client_s	*client	= msg->q.q_tag;
FN;
	client_delete(client);
}

static void post_key (void *m)
{
	sw_msg_s	*msg = m;
	unsigned	key = msg->q.q_passed_key;
	name_s		*name;
FN;
	name = name_find( &msg->sw_name);
	key_add(key, name, msg->q.q_tag, SW_LOCAL);
	node_send(key, name);
	request_honor(key, name, SW_REMOTE);
}

static void get_keys (void *m)
{
	sw_msg_s	*msg	= m;
	unsigned	to_key	= msg->q.q_passed_key;
	client_s	*client	= msg->q.q_tag;
	u8		request = msg->sw_request & SW_ALL;
	name_s		*name;
FN;
	if (msg->q.q_type & ONCE) {
		request |= JUST_ONE;
	}
	name = name_find( &msg->sw_name);
	request_add(to_key, name, client, request);
	key_send(to_key, name, request);
}

void all_keys (void *m)
{
	sw_msg_s	*msg	= m;
	unsigned	to_key	= msg->q.q_passed_key;
	client_s	*client	= msg->q.q_tag;
	name_s		*name;
FN;
	name = name_find( &msg->sw_name);
	request_add(to_key, name, client, SW_ALL);
	key_send(to_key, name, SW_ALL);
}

void local_key (void *m)
{
	sw_msg_s	*msg	= m;
	unsigned	to_key	= msg->q.q_passed_key;
	name_s		*name;
FN;
	name = name_find( &msg->sw_name);
	key_send_any(to_key, name, SW_LOCAL);
}

void remote_keys (void *m)
{
	sw_msg_s	*msg	= m;
	unsigned	to_key	= msg->q.q_passed_key;
	client_s	*client	= msg->q.q_tag;
	name_s		*name;
FN;
	name = name_find( &msg->sw_name);
	request_add(to_key, name, client, SW_REMOTE);
	key_send(to_key, name, SW_REMOTE);
}

void any_key (void *m)
{
	sw_msg_s	*msg	= m;
	unsigned	to_key	= msg->q.q_passed_key;
	name_s		*name;
FN;
	name = name_find( &msg->sw_name);
	key_send_any(to_key, name, SW_ALL);
}

static void next_key (void *m)
{
	sw_msg_s	*msg	= m;
	unsigned	to_key	= msg->q.q_passed_key;
	name_s		*name;
FN;
	name = name_find( &msg->sw_name);
	key_next(to_key, name, SW_ALL, msg->sw_sequence);
}

typedef struct register_type_s {
	type_s		tr_tag;
	method_f	tr_register;
} register_type_s;

typedef struct register_s {
	type_s		*r_type;
} register_s;

void register_client (void *m);

register_type_s	Register_type = {
				{ 1, 0 },
				register_client};

register_s	Register = { &Register_type.tr_tag};

void register_client (void *m)
{
	sw_msg_s	*msg = m;
	client_s	*client;
	ki_t		client_key;
	ki_t		reply_key;
	int		rc;
FN;
	reply_key = msg->q.q_passed_key;

	client = client_new( &msg->sw_name);
	if (!client) {
		destroy_key_tau(reply_key);
		return;
	}
	client_key = make_gate(client, RESOURCE | PASS_ANY);
	if (!client_key) {
		destroy_key_tau(reply_key);
		return;
	}
	msg->q.q_passed_key = client_key;
	rc = send_key_tau(reply_key, msg);
	if (rc) {
		destroy_key_tau(client_key);
		destroy_key_tau(reply_key);
		eprintf("register_client couldn't send client key %d\n", rc);
	}
}

int init_client (void)
{
	msg_s	msg;
	int	rc;
FN;
	msg.q.q_passed_key = make_gate( &Register,
					REQUEST | PASS_REPLY);
	if (!msg.q.q_passed_key) {
		eprintf("sw:init_client:create_gate");
	}
	rc = plug_key_tau(PLUG_CLIENT, &msg);
	if (rc) {
		eprintf("sw:init_client:plug_key %d", rc);
	}
	return rc;
}
