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

#include <style.h>
#include <tau/msg.h>
#include <tau/msg_util.h>
#include <tau/bicho.h>
#include <tau/tag.h>

#include <mystdlib.h>
#include <q.h>
#include <debug.h>
#include <eprintf.h>
#include <cmd.h>

typedef struct kbicho_s {
	dqlink_t	kb_link;
	char		kb_name[TAU_NAME];
	guid_t		kb_id;
	unint		kb_key;
} kbicho_s;

static d_q	Kbicho = static_init_dq(Kbicho);

#if 0
static unint find_process (char *name, u64 id)
{
	kbicho_s	*b = container(obj, kbicho_s, kb_link);
	bicho_msg_s	m;
	int		rc;

	printf("%s\n", name);
	m.ps_id = 0;
	for (;;) {
		m.m_method = BICHO_PS;
		rc = call_tau(b->kb_key, &m);
		if (rc) {
			return 0;
		}
		if (strncmp(m.ps_name, name, TAU_NAME) == 0) {
			if (id) {
				if (id == m.ps_id) {
				}
			} else {
			}
		}
		//pr_avatar(m.ps_name, m.ps_id, m.ps_num_waiters,
		//		m.ps_num_msgs, m.ps_dieing);
	}
	return 0;
}

static void lookup_bicho (char *name)
{
	foreach_dq();	//XXX:
}
#endif

static kbicho_s *lookup_bicho_by_id (guid_t id)
{
	kbicho_s	*bicho = NULL;

	for (;;) {
		next_dq(bicho, &Kbicho, kb_link);
		if (!bicho) return NULL;
		if (uuid_compare(id, bicho->kb_id) == 0) return bicho;
	}
}

static void who_am_i (kbicho_s *b)
{
	bicho_msg_s	m;
	int		rc;

	m.m_method = BICHO_WHO_AM_I;
	rc = call_tau(b->kb_key, &m);
	if (rc) {
		weprintf("who_am_i failed %d", rc);
		return;
	}
	strlcpy(b->kb_name, m.bi_name, TAU_NAME);
	uuid_copy(b->kb_id, m.bi_id);
	PRs(b->kb_name);
}

void kbicho (void *msg)
{
	bicho_msg_s	*m = msg;
	kbicho_s	*b;

	b = ezalloc(sizeof(*b));
	b->kb_key = m->q.q_passed_key;
	who_am_i(b);
	enq_dq( &Kbicho, b, kb_link);
}

static addr pr_kbicho (void *obj, void *data)
{
	kbicho_s	*b = container(obj, kbicho_s, kb_link);
	char		out_id[50];

	uuid_unparse(b->kb_id, out_id);
	printf("\t%s %s\n", b->kb_name, out_id);

	return 0;
}

int nsp (int argc, char *argv[])
{
	foreach_dq( &Kbicho, pr_kbicho, NULL);
	return 0;
}

static void pr_avatar (
	char	*name,
	u64	id,
	u32	num_waiters,
	u32	num_msgs,
	u8	dieing)
{
	printf("\t%*.*s/%llu%s %u %u\n",
			10, TAU_NAME, name, id,
			dieing ? "!" : "",
			num_waiters, num_msgs);
}

static addr ps_kbicho (void *obj, void *data)
{
	kbicho_s	*b = container(obj, kbicho_s, kb_link);
	bicho_msg_s	m;
	int		rc;

	printf("%s\n", b->kb_name);
	m.ps_id = 0;
	for (;;) {
		m.m_method = BICHO_NEXT_PID;
		rc = call_tau(b->kb_key, &m);
		if (rc) {
			return 0;
		}
		pr_avatar(m.ps_name, m.ps_id, m.ps_num_waiters,
				m.ps_num_msgs, m.ps_dieing);
	}
	return 0;
}

int psp (int argc, char *argv[])
{
	foreach_dq( &Kbicho, ps_kbicho, NULL);
	return 0;
}

#if 0
static addr map_keys (void *obj, void *data)
{
	return 0;
}

void get_name (char *name, char *path, unint len)
{
	char	c;

	while (*path == '/') {
		++path;
	}
	while (--len) {
		c = *path++;
		if (!c) break;
		if (c == '/') break;
		*name++ = c;
	}
	*name = '\0';
}

/*
 * keys [[/<node>/]<avatar>[/<id>]]
 */
int keysp (int argc, char *argv[])
{
	char	node_name[TAU_NAME+1];
	char	avatar_name[TAU_NAME+1];


	if (argc < 2) {
		avatar = CurrentAvatar;
	} else {
		path = argv[1];
		if (*path == '/') {	// node specified
			get_name(node_name, path, sizeof(node_name));
		}
	}
	return 0;
}
#endif

static unint get_process_key (kbicho_s *b, u64 id)
{
	bicho_msg_s	m;
	int		rc;

	m.m_method = BICHO_AVATAR;
	m.ps_id = id;
	rc = call_tau(b->kb_key, &m);
	if (rc) {
		return 0;
	}
	return m.q.q_passed_key;
}

static char *gate_type (unsigned type)
{
	switch (type & (ONCE | PERM)) {
	case 0:		return "RESOURCE";
	case ONCE:
	case ONCE|PERM:	return "REPLY   ";
	case PERM:	return "REQUEST ";
	default:	return "UNKNOWN KEY TYPE";
	}
}

static char *gate_pass (unsigned type)
{
	switch (type & PASS_ANY) {
	case 0:			return "";
	case PASS_REPLY:	return " PASS_REPLY";
	case PASS_OTHER:	return " PASS_OTHER";
	case PASS_ANY:		return " PASS_ANY  ";
	default:		return " BAD PASS RESTRICTION";
	}
}

static char *gate_data (unsigned type)
{
	switch (type & (RW_DATA)) {
	case 0:			return "";
	case READ_DATA:		return " READ_DATA ";
	case WRITE_DATA:	return " WRITE_DATA";
	case RW_DATA:		return " RW_DATA   ";
	default:		return " I give up";
	}
}

static void print_gate (u8 type, char *node_name, char *avatar, u64 pid, u64 tag)
{
	printf("%s%s /%s/%s/%lld tag=%llx\n",
		gate_type(type),
		gate_pass(type),
		node_name,
		avatar, pid,
		tag);
}

static void pr_key (bicho_msg_s *m)
{
	kbicho_s	*bicho;
	bicho_msg_s	mg;
	bicho_msg_s	mp;
	int		rc;

	bicho = lookup_bicho_by_id(m->key_node_id);
	if (!bicho) {
		printf("UNKNOWN NODE\n");
		return;
	}
	mg.m_method = BICHO_GATE;
	mg.gate_id  = m->key_id;
	rc = call_tau(bicho->kb_key, &mg);
	if (rc) {
		//XXX: Broken or old key
		//print_gate(0, bicho->kb_name, NULL, 0, 0);
		//printf("pr_gate call gate failed %d\n", rc);
		return;
	}
	mp.m_method = BICHO_PS;
	mp.ps_id    = mg.gate_avatar;
	rc = call_tau(bicho->kb_key, &mp);
	if (rc) {
		print_gate(mg.gate_type, bicho->kb_name, NULL,
			mg.gate_avatar, mg.gate_tag);
		printf("pr_gate call ps failed %d\n", rc);
		return;
	}
	printf("\t%3lld. ", m->key_index);
	print_gate(mg.gate_type, bicho->kb_name, mp.ps_name, mp.ps_id,
		mg.gate_tag);

	if (mg.gate_type & RW_DATA) {
		printf("\t    %s start=%llx length=%llu\n",
			gate_data(mg.gate_type), mg.gate_start, mg.gate_length);
	}
}

static void keys_process (kbicho_s *b, u64 id)
{
	bicho_msg_s	m;
	unint		key;
	int		rc;

	key = get_process_key(b, id);
	if (!key) return;

	m.key_index = 0;
	for (;;) {
		m.m_method = BICHO_AVATAR_KEYS;
		rc = call_tau(key, &m);
		if (rc) break;
		pr_key( &m);
		++m.key_index;
	}
	destroy_key_tau(key);
}

static addr keys_kbicho (void *obj, void *data)
{
	kbicho_s	*b = container(obj, kbicho_s, kb_link);
	bicho_msg_s	m;
	int		rc;

	printf("%s\n", b->kb_name);
	m.ps_id = 0;
	for (;;) {
		m.m_method = BICHO_NEXT_PID;
		rc = call_tau(b->kb_key, &m);
		if (rc) {
			return 0;
		}
		printf("    %.*s/%llu\n",
			TAU_NAME, m.ps_name, m.ps_id);
		keys_process(b, m.ps_id);
	}
	return 0;
}

int keysp (int argc, char *argv[])
{
	foreach_dq( &Kbicho, keys_kbicho, NULL);
	return 0;
}
