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
#include <string.h>
#include <uuid/uuid.h>

#include <eprintf.h>
#include <debug.h>

#include <tau/msg.h>
#include <tau/tag.h>
#include <tau/msg_util.h>
#include <tau/net.h>
#include <tau/switchboard.h>

#include <network.h>
#include <sw.h>

enum { NODE_POST, NODE_CONNECT, NODE_OPS };	// NODE_POST must be same
						// value as SW_KEY_SUPPLIED!
						// Should probably have a
						// common type they both
						// inherit from. Or does it
						// it need to be the same
						// as SW_POST???!!!

enum { ASSERT_POST_SUPPLIED = 1 / (NODE_POST == SW_KEY_SUPPLIED) };

typedef struct node_type_s {
	type_s		tn_tag;
	method_f	tn_connect;
	method_f	tn_post;
} node_type_s;

typedef struct sw_node_s {
	node_type_s	*n_type;
	unsigned	n_key;
	guid_t		n_id;
} sw_node_s;

static void node_destroy (void *m);
static void node_connect (void *m);
static void node_post    (void *m);

node_type_s	Node_type = {
			{ "Node", NODE_OPS, node_destroy },
			node_post,
			node_connect };

sw_node_s	Node[MAX_NODES];
sw_node_s	*NextNode = Node;

void node_pr (sw_node_s *node)
{
	char	uuid[40];

	uuid_unparse(node->n_id, uuid);
	printf("node: %s key: %d\n", uuid, node->n_key);
}

void node_dump (void)
{
	sw_node_s	*node;
FN;
	for (node = Node; node < NextNode; node++) {
		if (node->n_key) {
			node_pr(node);
		}
	}
}

void node_send (unsigned key, name_s *name)
{
	sw_node_s	*node;
FN;
	for (node = Node; node < NextNode; node++) {
		if (node->n_key) {
			dup_send_key(node->n_key, key, name, 0);
		}
	}
}

static sw_node_s *node_new (void)
{
	sw_node_s	*node;
FN;
	if (NextNode == &Node[MAX_NODES]) {
		eprintf("Too many nodes");
		return NULL;
	}
	node = NextNode++;
	node->n_type = &Node_type;
	node->n_key = 0;
	uuid_clear(node->n_id);

	return node;
}

static sw_node_s *node_add (unsigned key, guid_t id)
{
	sw_node_s	*node;
FN;
	node = node_new();
	if (!node) return NULL;

	node->n_key = key;
	uuid_copy(node->n_id, id);
node_pr(node);
	key_send_all(key, SW_LOCAL);
	return node;
}

static void node_destroy (void *m)
{
	msg_s		*msg = m;
	sw_node_s	*node = msg->q.q_tag;
FN;
	destroy_key_tau(node->n_key);
	zero(*node);
}

static void node_connect (void *m)
{
	net_msg_s	*msg = m;
	sw_node_s		*node = msg->q.q_tag;
FN;
	if (node->n_key) {
		printf("node_new: destroying key %d\n", node->n_key);
		destroy_key_tau(node->n_key);
		node->n_key = 0;
	}
	node->n_key = msg->q.q_passed_key;
	uuid_copy(node->n_id, msg->net_node_id);
	key_send_all(node->n_key, SW_LOCAL);
}

static void node_post (void *m)
{
	sw_msg_s	*msg = m;
	sw_node_s		*node = msg->q.q_tag;
	unsigned	key = msg->q.q_passed_key;
	name_s		*name;
FN;
	name = name_find( &msg->sw_name);
	key_add(key, name, node, SW_REMOTE);
	request_honor(key, name, SW_REMOTE);
}


typedef struct net_type_s {
	type_s		tnt_tag;
	method_f	tnt_new;
	method_f	tnt_connect;
} net_type_s;

typedef struct net_s {
	type_s		*nt_type;
} net_s;

static void net_new     (void *m);
static void net_connect (void *m);

enum { NET_NEW, NET_CONNECT, NET_OPS };

net_type_s	Net_type = {
			{ "Net", NET_OPS, 0 },
			net_new,
			net_connect};

net_s	Net = { &Net_type.tnt_tag};

guid_t		My_net_id;

static void net_new (void *m)
{
	net_msg_s	*msg = m;
	unsigned	to_key = msg->q.q_passed_key;
	sw_node_s	*node;
	ki_t		key;
	int		rc;
FN;
	node = node_new();
	key = make_gate(node, RESOURCE | PASS_OTHER);
	if (!key) {
		eprintf("net_new: make_gate failed");
		return;
	}
	msg->m_method = NET_CONNECT;
	msg->q.q_passed_key = key;
	uuid_copy(msg->net_id, My_net_id);
	rc = send_key_tau(to_key, msg);
	if (rc) {
		destroy_key_tau(key);
	}
	destroy_key_tau(to_key);
}

static void net_connect (void *m)
{
	net_msg_s	*msg = m;
	ki_t		to_key = msg->q.q_passed_key;
	sw_node_s	*node;
	ki_t		key;
	int		rc;
FN;
	node = node_add(to_key, msg->net_id);
	key = make_gate(node, RESOURCE | PASS_OTHER);
	if (!key) {
		eprintf("net_new: make_gate failed");
		return;
	}
	msg->m_method = NODE_CONNECT;
	msg->q.q_passed_key = key;
	uuid_copy(msg->net_id, My_net_id);
	rc = send_key_tau(to_key, msg);
	if (rc) {
		destroy_key_tau(key);
	}
}

int init_net (char *node_dev)
{
	net_msg_s	msg;
	ki_t		key;
	int		rc;
FN;
	strncpy((char *)&msg.net_dev_name, node_dev, BODY_SIZE);
	rc = my_node_id_tau( &msg);
	if (rc) {
		eprintf("sw:init:my_node_id_tau %d", rc);
	}
	memcpy( &My_net_id, msg.net_node_id, sizeof My_net_id);

	key = make_gate( &Net, REQUEST | PASS_OTHER);
	if (!key) {
		eprintf("sw:init net:create_gate");
	}
	msg.m_method = NET_NEW;
	msg.q.q_passed_key = key;
	uuid_copy(msg.net_id, My_net_id);
	rc = plug_key_tau(PLUG_NETWORK, &msg);
	if (rc) {
		eprintf("sw:init net:plug_key_tau %d", rc);
	}
	return rc;
}
