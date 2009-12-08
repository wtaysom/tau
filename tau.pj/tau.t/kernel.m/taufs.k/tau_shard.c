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
#include <linux/pagemap.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/switchboard.h>
#include <tau/fsmsg.h>

#define tMASK	tFS
#include <tau/debug.h>
#include <tau_fs.h>
#include <util.h>

static void open_spoke (void *msg);
static void create_spoke (void *msg);
static struct shard_type_s {
	type_s		st_tag;
	method_f	st_ops[SHARD_OPS];
} Shard_type = { { SHARD_OPS, NULL },
		{	open_spoke,
			create_spoke }};

static hub_s *hub_hash (shard_s *shard, u64 ino)
{
	void	*bucket;

	bucket = &shard->sh_hub_buckets[ino % HUB_BUCKETS];
	return container(bucket, hub_s, h_hash);
}

static hub_s *get_hub (shard_s *shard, u64 ino)
{
	hub_s	*prev;
	hub_s	*next;

	prev = hub_hash(shard, ino);
	for (;;) {
		next = prev->h_hash;
		if (!next) return NULL;
		if (next->h_tnode.t_ino == ino) {
			return next;
		}
		prev = next;
	}
}	

static void add_hub (shard_s *shard, hub_s *hub)
{
	hub_s	*prev;
	u64	ino = hub->h_tnode.t_ino;

	hub->h_shard = shard;
	prev = hub_hash(shard, ino);
	hub->h_hash  = prev->h_hash;
	prev->h_hash = hub;
}

hub_s *make_root_hub (shard_s *shard)
{
	const char	root[] = "/";
	hub_s	*hub;
	tau_inode_s	*t;

	hub = zalloc_tau(sizeof(*hub) + sizeof(root));
	if (!hub) return NULL;

	t = &hub->h_tnode;

	t->t_ino      = TAU_ROOT_INO;
	t->t_mode     = 040755;
	t->t_blksize  = TAU_BLK_SIZE;
	t->t_atime    = t->t_mtime = t->t_ctime = CURRENT_TIME;
	t->t_name_len = sizeof(root);

	init_dq( &hub->h_spokes);
	strcpy(hub->h_tnode.t_name, root);
	init_tree( &hub->h_dir, &Tau_dir_species,
			shard->sh_bag->bag_isuper, t->t_btree_root);

	add_hub(shard, hub);
	return hub;
}

static void free_spoke (spoke_s *spoke)
{
FN;
	rmv_dq( &spoke->ri_link);
	free_tau(spoke);
}

static void close_spoke (void *msg)
{
	msg_s	*m = msg;
	spoke_s	*spoke = m->q.q_tag;
FN;
	destroy_key_tau(spoke->ri_key);
	free_spoke(spoke);
}

static void register_spoke (void *msg)
{
	msg_s		*m = msg;
	spoke_s	*remote = m->q.q_tag;
FN;
	remote->ri_key = m->q.q_passed_key;
}

static void add_dir_entry (void *msg)
{
	fsmsg_s		*m = msg;
	spoke_s		*spoke = m->q.q_tag;
	hub_s		*hub = spoke->ri_hub;
	ki_t		reply_key = m->q.q_passed_key;
	unint		length;
	u8		name[TAU_FILE_NAME_LEN+1];
	int		rc;
FN;
	if (m->q.q_length > TAU_FILE_NAME_LEN) {
		length = TAU_FILE_NAME_LEN;
	} else {
		length = m->q.q_length;
	}
	rc = read_data_tau(reply_key, length, name, 0);
	if (rc) {
		eprintk("read_data_tau %d", rc);
		destroy_key_tau(reply_key);
		return;
	}
	rc = tau_insert_dir( &hub->h_dir, m->crt_child, name);
	if (rc) {
		eprintk("tau_insert_dir %d", rc);
		destroy_key_tau(reply_key);
		return;
	}
	rc = send_tau(reply_key, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		destroy_key_tau(reply_key);
		return;
	}
}

static void lookup_dir_entry (void *msg)
{
	fsmsg_s		*m = msg;
	spoke_s		*spoke = m->q.q_tag;
	hub_s		*hub = spoke->ri_hub;
	ki_t		reply_key = m->q.q_passed_key;
	unint		length;
	u8		name[TAU_FILE_NAME_LEN+1];
	u64		ino;
	int		rc;
FN;
	if (m->q.q_length > TAU_FILE_NAME_LEN) {
		length = TAU_FILE_NAME_LEN;
	} else {
		length = m->q.q_length;
	}
	rc = read_data_tau(reply_key, length, name, 0);
	if (rc) {
		eprintk("read_data_tau %d", rc);
		destroy_key_tau(reply_key);
		return;
	}
	ino = tau_find_dir( &hub->h_dir, name);
	if (!ino) {
		destroy_key_tau(reply_key);
		return;
	}
	m->crr_ino = ino;
	rc = send_tau(reply_key, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		destroy_key_tau(reply_key);
		return;
	}
}

static u64 map_phyblk (hub_s *hub, u64 blkno)
{
	tau_inode_s	*tnode = &hub->h_tnode;
	tau_extent_s	*extent;
	u64		last_blk;
	u64		offset;
	unint		i;
FN;
	last_blk = 0;
	extent = tnode->t_extent;
	for (i = 0; i < TAU_NUM_EXTENTS; extent++, i++) {
		if (extent->e_length == 0) return 0;
		if (extent->e_length < 0) {
			last_blk -= extent->e_length;
			if (blkno < last_blk) {
				eprintk("We don't handle holes yet %lld %lld\n",
					tnode->t_ino, blkno);
				return 0;
			}
		} else {
			last_blk += extent->e_length;
			if (blkno < last_blk) {
				offset = extent->e_length - (last_blk - blkno);
				return extent->e_start + offset;
			}
		}
	}
	return 0;
}

static int add_phyblk (hub_s *hub, u64 blkno, u64 phyno)
{
	tau_inode_s	*tnode = &hub->h_tnode;
	tau_extent_s	*extent;
	u64		last_blk;
	unint		i;
FN;
	last_blk = 0;
	extent = tnode->t_extent;
	for (i = 0; i < TAU_NUM_EXTENTS; extent++, i++) {
		if (extent->e_length == 0) {
			extent->e_length = 1;
			extent->e_start  = phyno;
			hub->h_dirty = TRUE;
			return 0;
		}
		if (extent->e_length < 0) {
			last_blk -= extent->e_length;
		} else {
			last_blk += extent->e_length;
			if (blkno < last_blk) {
				eprintk("We don't handle extents yet"
					" %lld %lld %lld\n",
					tnode->t_ino, blkno, phyno);
				return 0;
			}
		}
	}
	return 0;
}

static void prepare_write (void *msg)
{
	fsmsg_s		*m        = msg;
	spoke_s		*spoke    = m->q.q_tag;
	hub_s		*hub      = spoke->ri_hub;
	struct inode	*inode    = hub->h_shard->sh_bag->bag_isuper;
	ki_t		reply_key = m->q.q_passed_key;
	void		*data     = NULL;
	u64		blkno     = m->io_blkno;
	u64		phyno;
	int		rc;
FN;
	phyno = map_phyblk(hub, blkno);
	if (!phyno) {
		//XXX: we should really do delayed allocation
		phyno = tau_alloc_block(inode->i_sb, 1);
		if (!phyno) goto error;
		add_phyblk(hub, blkno, phyno);

		m->m_response = ZERO_FILL;
		rc = send_tau(reply_key, m);
		if (rc) {
			eprintk("send_tau %d", rc);
		}
		return;
	}
	data = tau_bget(inode->i_mapping, phyno);
	if (!data) goto error;

	rc = write_data_tau(reply_key, PAGE_CACHE_SIZE, data, 0);
	if (rc) {
		eprintk("write_data_tau %p", data);
		goto error;
	}
	m->m_response = DATA_FILL;
	rc = send_tau(reply_key, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		goto error;
	}
	tau_bput(data);
	return;
error:
	if (data) tau_bput(data);
	destroy_key_tau(reply_key);
}

static void flush_page (void *msg)
{
	fsmsg_s		*m        = msg;
	spoke_s		*spoke    = m->q.q_tag;
	hub_s		*hub      = spoke->ri_hub;
	struct inode	*inode    = hub->h_shard->sh_bag->bag_isuper;
	ki_t		reply_key = m->q.q_passed_key;
	void		*data     = NULL;
	u64		blkno     = m->io_blkno;
	u64		phyno;
	int		rc;
FN;
	phyno = map_phyblk(hub, blkno);
	if (!phyno) {
		eprintk("no block allocted for inode %lld at %lld",
			hub->h_tnode.t_ino, blkno);
		goto error;
	}
	data = tau_bget(inode->i_mapping, phyno);
	if (!data) goto error;

	rc = read_data_tau(reply_key, PAGE_CACHE_SIZE, data, 0);
	if (rc) {
		eprintk("read_data_tau %p", data);
		goto error;
	}
	tau_bdirty(data);
	rc = send_tau(reply_key, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		goto error;
	}
	tau_bput(data);
	return;
error:
	if (data) tau_bput(data);
	destroy_key_tau(reply_key);
}

static struct {
	type_s		spi_tag;
	method_f	spi_ops[SPOKE_OPS];
} Spoke_type = { { SPOKE_OPS, close_spoke },
			{ register_spoke,
			  add_dir_entry,
			  lookup_dir_entry,
			  prepare_write,
			  flush_page }};

static spoke_s *alloc_spoke (hub_s *hub)
{
	spoke_s	*spoke;
FN;
	spoke = zalloc_tau(sizeof(*spoke));
	if (!spoke) return NULL;

	spoke->ri_type = &Spoke_type.spi_tag;
	spoke->ri_hub  = hub;
	enq_dq( &hub->h_spokes, spoke, ri_link);
	return spoke;
}

hub_s *open_hub (shard_s *shard, u64 ino)
{		
	hub_s	*hub;
	int		rc;
FN;
	hub = get_hub(shard, ino);
	if (hub) return hub;

	rc = tau_find_hub( &shard->sh_inodes, ino, &hub);
	if (rc) {
		if (ino == TAU_ROOT_INO) {
			hub = make_root_hub(shard);
			if (!hub) return NULL;
		} else {
			return NULL;
		}
	}
	add_hub(shard, hub);
	return hub;
}

static void open_spoke (void *msg)
{
	fsmsg_s		*m      = msg;
	shard_s		*shard  = m->q.q_tag;
	ki_t		reply   = m->q.q_passed_key;
	ki_t		key     = 0;
	u64		ino     = m->fs_inode.fs_ino;
	spoke_s		*spoke = NULL;
	hub_s		*hub    = NULL;
	tau_inode_s	*tnode;
	int		rc;
FN;
	hub = open_hub(shard, ino);
	if (!hub) goto error;

	spoke = alloc_spoke(hub);
	if (!spoke) {
		rc = ENOMEM;
		goto error;
	}

	key = make_gate(spoke, RESOURCE | PASS_OTHER);
	if (!key) goto error;

	tnode = &hub->h_tnode;
	m->q.q_passed_key     = key;
	m->fs_inode.fs_ino    = tnode->t_ino;
	m->fs_inode.fs_size   = tnode->t_size;
	m->fs_inode.fs_blocks = tnode->t_blocks;
	m->fs_inode.fs_mode   = tnode->t_mode;
	m->fs_inode.fs_uid    = tnode->t_uid;
	m->fs_inode.fs_gid    = tnode->t_gid;
	m->fs_inode.fs_atime  = tnode->t_atime.tv_sec;
	m->fs_inode.fs_mtime  = tnode->t_mtime.tv_sec;
	m->fs_inode.fs_ctime  = tnode->t_ctime.tv_sec;
	rc = send_key_tau(reply, m);
	if (rc) {
		eprintk("send_path_tau %d", rc);
		goto error;
	}
	return;
error:
	if (key) {
		destroy_key_tau(key);
	} else {
		if (spoke) {
			rmv_dq( &spoke->ri_link);
			free_tau(spoke);
		}
		if (hub) {
			if (is_empty_dq( &hub->h_spokes)) {
				free_tau(hub);
			}
		}
	}
	destroy_key_tau(reply);
}

static u64 alloc_ino (shard_s *shard)
{
	bag_s		*bag  = shard->sh_bag;
	tau_bag_s	*tbag = bag->bag_block;
	tau_shard_s	*tshard = &shard->sh_disk;
	u64		ino;
	int		rc;
FN;
	ino = tshard->ts_next_ino;
	tshard->ts_next_ino += tbag->tb_num_shards;

	rc = tau_delete_shard( &bag->bag_tree, tshard);
	if (rc) {
		eprintk("tau_delete_shard %d for shard no %d",
			rc, tshard->ts_shard_no);
		return 0;
	}
	rc = tau_insert_shard( &bag->bag_tree, tshard);
	if (rc) {
		eprintk("tau_insert_shard %d for shard no %d",
			rc, tshard->ts_shard_no);
		return 0;
	}
	return ino;
}	

static void create_spoke (void *msg)
{
	fsmsg_s		*m      = msg;
	shard_s		*shard  = m->q.q_tag;
	ki_t		reply   = m->q.q_passed_key;
	ki_t		key     = 0;
	hub_s		*hub    = NULL;
	spoke_s		*spoke  = NULL;
	u64		ino;
	int		rc;
FN;
	hub = alloc_hub(m->q.q_length);	//XXX: check length
	if (!hub) goto error;

	spoke = alloc_spoke(hub);
	if (!spoke) goto error;

	rc = read_data_tau(reply, m->q.q_length, hub->h_tnode.t_name, 0);
	if (rc) {
		printk("read_data_tag %d", rc);
		goto error;
	}

	ino = alloc_ino(shard);
	if (!ino) goto error;

	hub->h_tnode.t_ino     = ino;
	hub->h_tnode.t_parent  = m->crt_parent;
	hub->h_tnode.t_mode    = m->crt_mode;
	hub->h_tnode.t_link    = 0;
	hub->h_tnode.t_blksize = BLK_SIZE;
	hub->h_tnode.t_size    = 0;
	hub->h_tnode.t_blocks  = 0;
	hub->h_tnode.t_uid     = m->crt_uid;
	hub->h_tnode.t_gid     = m->crt_gid;
	hub->h_tnode.t_atime   =
	hub->h_tnode.t_mtime   =
	hub->h_tnode.t_ctime   = CURRENT_TIME;//XXX: should we set it here?

	add_hub(shard, hub);

	rc = tau_insert_hub( &shard->sh_inodes, hub);
	if (rc) {
		printk("tau_insert_hub %d", rc);
		goto error;
	}
	key = make_gate(spoke, RESOURCE | PASS_ANY);
	if (!key) goto error;	//XXX: have to undo tau_insert_hub

	m->crr_ino  = ino;
	m->m_method = 0;
	m->q.q_passed_key = key;
	rc = send_key_tau(reply, m);
	if (rc) {
		printk("send_key_tay %d", rc);
		goto error;
	}
	return;

error:
	if (key) {
		destroy_key_tau(key);
	} else {
		if (spoke) {
			free_spoke(spoke);
		}
		if (hub) {
			free_hub(hub);
		}
	}
	destroy_key_tau(reply);
}

static void kill_shard (shard_s *shard)
{
	die_tau(shard->sh_avatar);

	free_tau(shard);
}

static void shard_receive (int rc, void *msg)
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

static d_q	Shard_list = static_init_dq(Shard_list);
//XXX: put a lock on the shard list

static shard_s *find_shard (guid_t vol_id, u32 shard_no, u32 replica_no)
{
	shard_s	*s;

	for (s = NULL;;) {
		next_dq(s, &Shard_list, sh_list);
		if (!s) return NULL;
		if (uuid_is_equal(vol_id, s->sh_vol_id)
		    && (shard_no   == s->sh_disk.ts_shard_no)
		    && (replica_no == s->sh_disk.ts_replica_no)) {
			return s;
		}
	}
}

static int add_shard (shard_s *s)
{
	if (find_shard(s->sh_vol_id,
			s->sh_disk.ts_shard_no,
			s->sh_disk.ts_replica_no)) {
		return -1;
	}
	s->sh_inuse = 1;
	enq_dq( &Shard_list, s, sh_list);
	return 0;
}

static void rmv_shard (shard_s *s)
{
	if (is_qmember(s->sh_list)) {
		--s->sh_inuse;	// Wrong - see release_shard
		rmv_dq( &s->sh_list);
	}
	if (!s->sh_inuse) {
		kill_shard(s);
	}
}

static shard_s *get_shard (guid_t vol_id, u32 shard_no, u32 replica)
{
	shard_s	*s;

	s = find_shard(vol_id, shard_no, replica);
	if (!s) return NULL;

	++s->sh_inuse;
	return s;
}

static void release_shard (shard_s *s)
{
	--s->sh_inuse;
	if (!s->sh_inuse) {
		rmv_dq( &s->sh_list);	// Wrong - see rmv_shard
		kill_shard(s);
	}
}

static shard_s *alloc_shard (
	guid_t		vol_id,
	tau_shard_s	*tshard,
	u64		instance)
{
	char	name[TAU_NAME];
	shard_s	*shard;
	int	rc;

	shard = zalloc_tau(sizeof(shard_s));
	if (!shard) return NULL;

	uuid_copy(shard->sh_vol_id, vol_id);
	shard->sh_disk = *tshard;
	add_shard(shard);

	snprintf(name, sizeof(name), "shard%lld.%d.%d",
			instance,
			shard->sh_disk.ts_shard_no,
			shard->sh_disk.ts_replica_no);

	shard->sh_avatar = init_msg_tau(name, shard_receive);

	rc = sw_register(name);
	if (rc) {
		rmv_shard(shard);
		return NULL;
	}
	return shard;
}

shard_s *new_shard (bag_s *bag, tau_shard_s *tau_shard)
{
	shard_name_s	shard_name;
	shard_s		*shard;
	ki_t		key;
	int		rc;
FN;
	shard = alloc_shard(bag->bag_block->tb_guid_vol, tau_shard,
				bag->bag_instance);
	if (!shard) return NULL;

	shard->sh_bag = bag;

	uuid_copy(shard_name.sn_guid, shard->sh_vol_id);
	shard_name.sn_shard_no   = shard->sh_disk.ts_shard_no;
	shard_name.sn_replica_no = shard->sh_disk.ts_replica_no;

		//XXX: Should we get a separate inode
		// for each replica that we own? The one
		// inode would be used for all meta-data
		// blocks for that shard replica
	init_tree( &shard->sh_inodes, &Tau_hub_species,
			bag->bag_isuper, tau_shard->ts_root);

	shard->sh_type = &Shard_type.st_tag;
	key = make_gate(shard, REQUEST | PASS_ANY);
	if (!key) goto error;

	rc = sw_post_v(sizeof(shard_name), &shard_name, key);
	if (rc) goto error;

	exit_tau();
	return shard;
error:
	rmv_shard(shard);
	exit_tau();
	return NULL;
}

void init_shards (bag_s *bag)
{
	u32		num_shards = bag->bag_block->tb_num_shards;
	u32		num_replicas = bag->bag_block->tb_num_replicas;
	tau_shard_s	tau_shard;
	unint		i;
	unint		j;
	int		rc;
FN;
	for (i = 0; i < num_shards; i++) {
		for (j = 0; j < num_replicas; j++) {
			tau_shard.ts_shard_no   = i;
			tau_shard.ts_replica_no = j;
			rc = tau_find_shard( &bag->bag_tree, &tau_shard);
			if (rc) continue;	// This seems wrong!

			new_shard(bag, &tau_shard);
		}
	}
}

void free_shards (bag_s *bag)
{
	u32		num_shards = bag->bag_block->tb_num_shards;
	u32		num_replicas = bag->bag_block->tb_num_replicas;
	shard_s		*shard;
	unint		i;
	unint		j;
FN;
	for (i = 0; i < num_shards; i++) {
		for (j = 0; j < num_replicas; j++) {

			shard = get_shard(bag->bag_block->tb_guid_vol, i, j);
			if (!shard) continue;

			rmv_shard(shard);
			release_shard(shard);
		}
	}
}

