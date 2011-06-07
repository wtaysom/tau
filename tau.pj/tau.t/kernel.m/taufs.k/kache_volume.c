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
#include <linux/module.h>

#include <style.h>
#include <kache.h>
#include <tau/debug.h>
#include <tau/switchboard.h>
#include <util.h>

static void replica_key (void *msg);
static struct {
	type_s		rst_tag;
	method_f	rst_ops[SW_REPLY_MAX];
} Replica_sw_type = { { "Replica_sw", SW_REPLY_MAX, NULL },
			{ replica_key }};

static d_q	Volume_list = static_init_dq(Volume_list);
//XXX: put a lock on the volume list

static volume_s *find_volume (guid_t guid)
{
	volume_s	*v;

	for (v = NULL;;) {
		next_dq(v, &Volume_list, vol_list);
		if (!v) return NULL;
		if (uuid_is_equal(guid, v->vol_guid)) {
			return v;
		}
	}
}

int add_volume (volume_s *v)
{
	if (find_volume(v->vol_guid)) return -1;
	v->vol_inuse = 1;
	enq_dq( &Volume_list, v, vol_list);
	return 0;
}

void rmv_volume (volume_s *v)
{
	if (is_qmember(v->vol_list)) {
		--v->vol_inuse;
		rmv_dq( &v->vol_list);
	}
}

volume_s *get_volume (guid_t guid)
{
	volume_s	*v;

	v = find_volume(guid);
	if (!v) return NULL;

	++v->vol_inuse;
	return v;
}

void put_volume (volume_s *v)
{
	--v->vol_inuse;
}

static void replica_key (void *msg)
{
	sw_msg_s	*m = msg;
	replica_s	*r =m->q.q_tag;

	r->rp_key = m->q.q_passed_key;
}

static void replica_receive (int rc, void *msg)
{
	struct object_s {
		type_s	*obj_type;
	} *obj;
	msg_s	*m = msg;
	type_s	*type;
FN;
	//XXX: Need to combine kernel receive functions

	obj  = m->q.q_tag;
	type = obj->obj_type;
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
		type->ty_destroy(m);
		return;
	}
	eprintk("err = %d", rc);
}

static replica_s *alloc_replica (
	volume_s	*volume,
	u32		shard_no,
	u32		replica_no)
{
	char		name[TAU_NAME];
	replica_s	*replica;
	int		rc;

	replica = zalloc_tau(sizeof(shard_s));
	if (!replica) return NULL;

	uuid_copy(replica->rp_vol_id, volume->vol_guid);
	replica->rp_volume     = volume;
	replica->rp_shard_no   = shard_no;
	replica->rp_replica_no = replica_no;

	snprintf(name, sizeof(name), "replica%lld.%d.%d",
			volume->vol_instance, shard_no, replica_no);

	replica->rp_avatar = init_msg_tau(name, replica_receive);

	rc = sw_register(name);
	if (rc) {
		free_tau(replica);
		return NULL;
	}
	return replica;
}

static replica_s *new_replica (
	volume_s	*volume,
	u32		shard_no,
	u32		replica_no)
{
	shard_name_s	shard_name;
	replica_s	*replica;
	ki_t		key;
	int		rc;
FN;
	replica = alloc_replica(volume, shard_no, replica_no);
	if (!replica) return NULL;

	uuid_copy(shard_name.sn_guid, replica->rp_vol_id);
	shard_name.sn_shard_no   = shard_no;
	shard_name.sn_replica_no = replica_no;

	replica->rp_sw_type = &Replica_sw_type.rst_tag;
	key = make_gate( &replica->rp_sw_type, REQUEST | PASS_OTHER);
	rc = sw_request_v(sizeof(shard_name), &shard_name, key);
	if (rc) goto error;
	exit_tau();

	return replica;
error:
	free_tau(replica);
	exit_tau();
	return NULL;
}

static void free_replica (replica_s *replica)
{
	die_tau(replica->rp_avatar);

	free_tau(replica);
}

replica_s *vol_replica (volume_s *v, u64 ino)
{
	replica_s	**replicas;
	replica_s	*r;
	unint	i;
FN;
	i = ino & (v->vol_num_shards - 1);
	replicas = v->vol_replicas[i];
	r = replicas[0];
	return r;
}

int init_replica_matrix (volume_s *v)
{
	static u64	instance = 0;

	unint		num_shards   = v->vol_num_shards;
	unint		num_replicas = v->vol_num_replicas;
	replica_s	**replicas;
	replica_s	*r;
	unint		i;
	unint		j;
	int		rc = 0;
FN;
	v->vol_replicas = zalloc_tau(num_shards * sizeof(replica_s **));
	if (!v->vol_replicas) return ENOMEM;

	v->vol_instance = ++instance;

	for (i = 0; i < num_shards; i++) {
		replicas = zalloc_tau(num_replicas * sizeof(replica_s *));
		if (!replicas) return ENOMEM;

		v->vol_replicas[i] = replicas;
		for (j = 0; j < num_replicas; j++) {
			r = new_replica(v, i, j);
			if (!r) return -1;
			replicas[j] = r;
		}
	}
	return rc;
}

void cleanup_replica_matrix (volume_s *v)
{
	unint		num_shards   = v->vol_num_shards;
	unint		num_replicas = v->vol_num_replicas;
	replica_s	**replicas;
	replica_s	*r;
	unint		i;
	unint		j;
FN;
	if (!v->vol_replicas) return;

	for (i = 0; i < num_shards; i++) {
		replicas = v->vol_replicas[i];
		if (!replicas) continue;

		v->vol_replicas[i] = NULL;
		for (j = 0; j < num_replicas; j++) {
			r = replicas[j];
			if (!r) continue;

			replicas[j] = NULL;
			free_replica(r);
		}
		free_tau(replicas);
	}
	free_tau(v->vol_replicas);
}

replica_s *pick_replica (volume_s *v)
{
	unint		n;
	replica_s	**replicas;
	replica_s	*r;
FN;
	do {
		n = v->vol_next_replica++;
		if (v->vol_next_replica >= v->vol_num_shards) {
			v->vol_next_replica = 0;
		}
		replicas = v->vol_replicas[n];
		r = replicas[0];
	} while (!r->rp_key);
	return r;
}
