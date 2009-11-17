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

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/utsname.h>

#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/tag.h>
#include <tau/bicho.h>

#include <msg_debug.h>
#include <msg_internal.h>
#include <util.h>


static void who_am_i        (void *msg);
static void ps              (void *msg);
static void next_pid        (void *msg);
static void lookup_avatar   (void *msg);
static void find_gate_by_id (void *msg);
struct bicho_sw_type_s {
	type_s		bst_tag;
	method_f	bst_methods[BICHO_SW_NUM];
} 	Bicho_sw_type = { { BICHO_SW_NUM, NULL }, {
				who_am_i,
				ps,
				next_pid,
				lookup_avatar,
				find_gate_by_id }};

typedef struct bicho_sw_s {
	struct bicho_sw_type_s	*bw_type;
} bicho_sw_s;

bicho_sw_s	Bicho_sw = { &Bicho_sw_type };


static void avatar_cleanup (void *msg);
static void avatar_stat    (void *msg);
static void avatar_keys    (void *msg);
static void avatar_gates   (void *msg);
struct bicho_avatar_type_s {
	type_s		bct_tag;
	method_f	bct_methods[BICHO_AVATAR_NUM];
} Bicho_avatar_type = { { BICHO_AVATAR_NUM, avatar_cleanup }, {
				avatar_stat,
				avatar_keys,
				avatar_gates }};

typedef struct bicho_avatar_s {
	struct bicho_avatar_type_s	*bc_type;
	avatar_s			*bc_avatar;
} bicho_avatar_s;

bool		Stop_bicho = TRUE;

static void who_am_i (void *msg)
{
	bicho_msg_s	*m = msg;
	unint		reply = m->q.q_passed_key;
	int		rc;
FN;
	strncpy(m->bi_name, system_utsname.nodename, TAU_NAME);
	Net_node_id(0, m->bi_id);
	rc = send_tau(reply, m);
	if (rc) {
		eprintk("who_am_i send_tau error=%d", rc);
		destroy_key_tau(reply);
	}
}

static addr ps_avatar (void *obj, void *msg)
{
	avatar_s	*avatar = container(obj, avatar_s, av_link);
	bicho_msg_s	*m = msg;

	if (avatar->av_id == m->ps_id) {
		strncpy(m->ps_name, avatar->av_name, TAU_NAME);
		m->ps_id          = avatar->av_id;
		m->ps_num_waiters = cnt_dq( &avatar->av_waiters);
		m->ps_num_msgs    = cnt_dq( &avatar->av_msgq);
		m->ps_dieing      = avatar->av_dieing;
		return 1;
	}
	return 0;
}

static void ps (void *msg)
{
	bicho_msg_s	*m = msg;
	unint		reply = m->q.q_passed_key;
	int		rc;
FN;
	rc = foreach_dq( &Avatars, ps_avatar, msg);
	if (rc) {
		rc = send_tau(reply, m);
		if (rc) {
			eprintk("ps send_tau error=%d", rc);
			destroy_key_tau(reply);
		}
	} else {
		destroy_key_tau(reply);
	}
}

static void next_pid (void *msg)
{
	bicho_msg_s	*m = msg;
	unint		reply = m->q.q_passed_key;
	avatar_s	*av;
	int		rc;
FN;
	for (av = NULL;;) {
		next_dq(av, &Avatars, av_link);
		if (!av) goto nothing_found;
		if (!m->ps_id) break;
		if (m->ps_id == av->av_id) {
			next_dq(av, &Avatars, av_link);
			if (!av) goto nothing_found;
			break;
		}
	}
	strncpy(m->ps_name, av->av_name, TAU_NAME);
	m->ps_id          = av->av_id;
	m->ps_num_waiters = cnt_dq( &av->av_waiters);
	m->ps_num_msgs    = cnt_dq( &av->av_msgq);
	m->ps_dieing      = av->av_dieing;
	rc = send_tau(reply, m);
	if (rc) {
		eprintk("next_pid send_tau error=%d", rc);
		goto nothing_found;
	}
	return;
nothing_found:
	destroy_key_tau(reply);
}

static addr key_for_avatar (void *obj, void *msg)
{
	bicho_msg_s	*m = msg;
	avatar_s	*av = container(obj, avatar_s, av_link);
	bicho_avatar_s	*bc;
	ki_t		reply;
	int		rc;

	if (av->av_id == m->ps_id) {
FN;
		reply = m->q.q_passed_key;

		bc = zalloc_tau(sizeof(*bc));
		if (!bc) {
			eprintk("zalloc_tau failed");
			destroy_key_tau(reply);
		}
		bc->bc_type    = &Bicho_avatar_type;
		bc->bc_avatar = av;
		++av->av_inuse;

		m->q.q_passed_key = make_gate(bc, RESOURCE | PASS_REPLY);
		rc = send_key_tau(reply, m);
		if (rc) {
			eprintk("ps send_key_tau error=%d", rc);
			destroy_key_tau(reply);
		}
		return 1;
	}
	return 0;
}

static void lookup_avatar (void *msg)
{
	bicho_msg_s	*m = msg;
	int		rc;
FN;
	rc = foreach_dq( &Avatars, key_for_avatar, msg);
	if (!rc) {
		destroy_key_tau(m->q.q_passed_key);
	}
}

static void find_gate_by_id (void *msg)
{
	bicho_msg_s	*m = msg;
	datagate_s	*gate;
	int		rc;
FN;
	gate = lookup_gate(m->gate_id);
	if (!gate) {
		eprintk("coulnd't find %llx", m->gate_id);
		destroy_key_tau(m->q.q_passed_key);
		return;
	}
	m->gate_tag    = (addr)gate->gt_tag;
	m->gate_avatar = gate->gt_avatar->av_id;
	m->gate_type   = gate->gt_type;
	if (gate->gt_type & RW_DATA) {
		m->gate_start  = gate->dg_start;
		m->gate_length = gate->dg_length;
	} else {
		m->gate_start  = 0;
		m->gate_length = 0;
	}
	rc = send_tau(m->q.q_passed_key, m);
	if (rc) {
		eprintk("send_tau failed %d", rc);
		destroy_key_tau(m->q.q_passed_key);
	}
}

static void avatar_cleanup (void *msg)
{
	bicho_msg_s	*m = msg;
	bicho_avatar_s	*bc = m->q.q_tag;
	avatar_s	*av = bc->bc_avatar;
FN;
	--av->av_inuse;
	if (!av->av_inuse && av->av_dieing) {
		//XXX: cleanup properly - need to modify death alg
	}
}

static void avatar_stat (void *msg)
{
	bicho_msg_s	*m = msg;
	bicho_avatar_s	*bc = m->q.q_tag;
	avatar_s	*av = bc->bc_avatar;
	int		rc;
FN;
	strncpy(m->ps_name, av->av_name, TAU_NAME);
	m->ps_id          = av->av_id;
	m->ps_num_waiters = cnt_dq( &av->av_waiters);
	m->ps_num_msgs    = cnt_dq( &av->av_msgq);
	m->ps_dieing      = av->av_dieing;

	rc = send_key_tau(m->q.q_passed_key, m);
	if (rc) {
		eprintk("send_key_tau=%d", rc);
		destroy_key_tau(m->q.q_passed_key);
	}
}

static void avatar_keys (void *msg)
{
	bicho_msg_s	*m = msg;
	bicho_avatar_s	*bc = m->q.q_tag;
	avatar_s	*av = bc->bc_avatar;
	key_s		key;
	unint		i;
	int		rc;
FN;
	for (i = m->key_index; i < MAX_KEYS; i++) {
		rc = read_key(av, i, &key);
		if (!rc) {
			m->key_index      = i;	
			m->key_length     = key.k_length;
			m->key_node_index = key.k_node;
			m->key_type       = key.k_type;
			m->key_reserved   = 0;
			m->key_id         = key.k_id;
			Net_node_id(key.k_node, m->key_node_id);

			rc = send_tau(m->q.q_passed_key, m);
			if (rc) {
				eprintk("send_tau=%d", rc);
				destroy_key_tau(m->q.q_passed_key);
			}
			return;
		}
	}
	destroy_key_tau(m->q.q_passed_key);
}

static void avatar_gates (void *msg)
{
	bicho_msg_s	*m = msg;
	bicho_avatar_s	*bc = m->q.q_tag;
	avatar_s	*av = bc->bc_avatar;
	datagate_s	*gate;
	int		rc;
FN;
	for (gate = NULL;;) {
		next_dq(gate, &av->av_gates, gt_siblings);
		if (!gate) goto nothing_found;
		if (!m->gate_id) break;
		if (m->gate_id == gate->gt_id) {
			next_dq(gate, &av->av_gates, gt_siblings);
			if (!gate) goto nothing_found;
			break;
		}
	}
	m->gate_id   = gate->gt_id;
	m->gate_tag  = (addr)gate->gt_tag;
	m->gate_type = gate->gt_type;
	if (gate->gt_type & (READ_DATA | WRITE_DATA)) {
		m->gate_start  = gate->dg_start;
		m->gate_length = gate->dg_length;
	} else {
		m->gate_start  = 0;
		m->gate_length = 0;
	}
	rc = send_tau(m->q.q_passed_key, m);
	if (rc) {
		eprintk("send_tau=%d", rc);
		goto nothing_found;
	}
	return;
nothing_found:
	destroy_key_tau(m->q.q_passed_key);
}

// Need to worry about smp locks
// Need to setup local switchboard requests.
//
// May want to do what I did in Oryx/Pecos - setup a thread for this guy
// and do a normal receive.
static void bicho_receive (int rc, void *msg)
{
	msg_s		*m = msg;
	object_s	*obj;
	type_s		*type;
FN;
	obj = m->q.q_tag;
if (!obj) bug("tag is null");
	type  = obj->o_type;
if (!type) bug("type is null %p", obj);
	if (!rc) {
		if (m->m_method < type->ty_num_methods) {
			type->ty_method[m->m_method](m);
			return;
		}
		printk(KERN_INFO "bad method %u >= %u\n",
			m->m_method, type->ty_num_methods);
		if (m->q.q_passed_key) {
			destroy_key_tau(m->q.q_passed_key);
		}
		return;	
	}
	if (rc == DESTROYED) {
		if (type->ty_destroy) type->ty_destroy(m);
		return;
	}
	printk(KERN_INFO "bicho_receive err = %d\n", rc);
}

static avatar_s	*Bicho_avatar;

void start_bicho (void *not_used)
{
	ki_t	key;
	int	rc;
FN;
	Bicho_avatar = init_msg_tau("kbicho", bicho_receive);
	if (!Bicho_avatar) {
		eprintk("init_bicho failed");
		return;
	}
	mod_put();	// Becuase bicho is part of msg module, need to
			// account for try_module_get so we can unload.
	Stop_bicho = FALSE;

	rc = sw_register("kbicho");
	if (rc) return;

	key = make_gate( &Bicho_sw, REQUEST | PASS_OTHER);
	if (!key) return;

	sw_post("kbicho", key);
}

void init_bicho (void)
{
	static struct work_struct	start;
FN;
	INIT_WORK( &start, start_bicho, NULL);

	schedule_work( &start);
}

void stop_bicho (void)
{
FN;
	if (Stop_bicho) return;
	Stop_bicho = TRUE;
	set_current_state(TASK_INTERRUPTIBLE);
	if (Bicho_avatar) {
		die_tau(Bicho_avatar);
		Bicho_avatar = NULL;
	}
}
