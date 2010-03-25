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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/utsname.h>
#include <linux/pagemap.h>

#include <style.h>
#include <tau/msg.h>
#include <tau/fsmsg.h>
#include <tau/switchboard.h>
#include <tau/debug.h>
#include <tau/disk.h>

#include <util.h>
#include "tau_fs.h"
#include <kache.h>	//XXX:delete

avatar_s	*Fs_avatar;

u64 tau_alloc_block (struct super_block *sb, unint num_blocks)
{
	bag_s		*bag  = sb->s_fs_info;
	tau_bag_s	*tbag = bag->bag_block;
	u64		pblk;
FN;
	pblk = tbag->tb_nextblock;
	tbag->tb_nextblock += num_blocks;
	tau_blog(tbag);
	return pblk;	
}

void tau_free_block (struct super_block *sb, u64 blkno)
{
FN;
}


static int not_valid (void)
{
	return -EPERM;
}

static struct file_operations bag_dir_ops =
{
};

static struct inode_operations bag_inode_ops =
{
	create:	(int(*)(struct inode *, struct dentry *,
			int, struct nameidata *))not_valid,
	lookup:	NULL,		// This blocks most operations.
	link:	(int(*)(struct dentry *,
			struct inode *, struct dentry *))not_valid,
	unlink:	(int(*)(struct inode *, struct dentry *))not_valid,
	mkdir:	(int(*)(struct inode *, struct dentry *,
			int))not_valid,
	rmdir:	(int(*)(struct inode *, struct dentry *))not_valid,
	rename:	(int(*)(struct inode *, struct dentry *,
			struct inode *, struct dentry *))not_valid,
	setattr:(int(*)(struct dentry *, struct iattr *))not_valid,
};

static struct address_space_operations bag_address_ops =
{
	.readpage	= tau_readpage,
	.invalidatepage = tau_invalidatepage,
};

static void ino_write (void *msg)
{
FN;
}

static void file_read (void *msg)
{
FN;
}

static void file_write (void *msg)
{
FN;
}

static void *alloc_dir_list (void)
{
	dir_list_s	*dl;
FN;
	dl = (dir_list_s *)__get_free_page(GFP_KERNEL | __GFP_HIGHMEM);
	if (!dl) {
		eprintk("no memory");
	}
	dl->dl_next = 0;
	return dl;
}

/*XXX:static*/ void free_dir_list (void *dl)
{
FN;
	free_page((addr)dl);
}

static int fill_dir_list (
	dir_list_s	*dl,
	char		*name,
	u64		ino,
	u64		pos,
	u8		type)
{
	unint	size;
	unint	i;

	size = strlen(name) + 1;
	size += sizeof(ino);
	size += sizeof(pos);
	//size += sizeof(d->d_type);

	if (size > DIR_LIST_LIMIT - dl->dl_next) return 0;

	i = dl->dl_next;
	dl->dl_next += size;
	PACK(dl->dl_list, ino, i);
	PACK(dl->dl_list, pos, i);
	//PACK(dl->dl_list, d->d_type, i);
	strcpy( &dl->dl_list[i], name);
	return size;
}

static void file_readdir (void *msg)
{
	fsmsg_s		*m = msg;
	file_s		*file = m->q.q_tag;
	unint		reply_key = m->q.q_passed_key;
	dir_list_s	*dl;
	unint		size;
	u8		name[TAU_FILE_NAME_LEN+1];
	u64		pos;
	u64		ino;
	int		rc;
FN;
	dl = alloc_dir_list();
	if (!dl) {
		destroy_key_tau(reply_key);
		return;
	}
	pos = m->rd_cookie;
	for (;;) {
		rc = tau_next_dir( &file->f_tree, pos, &pos, name, &ino);
		if (rc) break;
		size = fill_dir_list(dl, name, ino, pos, 0);//XXX: need to set type
		if (!size) break;
	}
	rc = write_data_tau(reply_key, sizeof(*dl), dl, 0);
	if (rc) {
		eprintk("write_data_tau %d", rc);
		destroy_key_tau(reply_key);
		return;
	}
	m->rd_cookie = pos;
	rc = send_tau(reply_key, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		destroy_key_tau(reply_key);
	}
}

static void file_destroyed (void *msg)
{
	msg_s			*m = msg;
	file_s			*file = m->q.q_tag;
FN;
	free_tau(file);
}

struct file_type_s {
	type_s		ft_tag;
	method_f	ft_ops[FILE_OPS];
} File_type = { { "File", FILE_OPS, file_destroyed },
		{ ino_write,
		  file_read,
		  file_write,
		  file_readdir }};		

static void stat_bag (void *msg)
{
	fsmsg_s		*m       = msg;
	ki_t		replykey = m->q.q_passed_key;
	bag_s		*bag     = m->q.q_tag;
	tau_bag_s	*tbag    = bag->bag_block;
	int		rc;
FN;
	uuid_copy(m->bag_guid_vol, tbag->tb_guid_vol);
	uuid_copy(m->bag_guid_bag, tbag->tb_guid_bag);
	m->bag_state        = tbag->tb_state;
	m->bag_num_blocks   = tbag->tb_numblocks;
	m->bag_num_shards   = tbag->tb_num_shards;
	m->bag_num_replicas = tbag->tb_num_replicas;
	rc = send_tau(replykey, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		destroy_key_tau(replykey);
	}
}

static void get_shard (void *msg)
{
	fsmsg_s		*m       = msg;
	ki_t		replykey = m->q.q_passed_key;
	bag_s	*b       = m->q.q_tag;
	tau_shard_s	tau_shard;
	int		rc;
FN;
	tau_shard.ts_shard_no   = m->sh_shard_no;
	tau_shard.ts_replica_no = m->sh_replica_no;
	rc = tau_find_shard( &b->bag_tree, &tau_shard);
	if (rc) {
		eprintk("tau_find_shard %d", rc);
		destroy_key_tau(replykey);
	}

	rc = send_tau(replykey, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		destroy_key_tau(replykey);
	}
}

static void put_shard (void *msg)
{
	fsmsg_s		*m       = msg;
	ki_t		replykey = m->q.q_passed_key;
	bag_s	*bag     = m->q.q_tag;
	tau_shard_s	tau_shard;
	int		rc;
FN;
	if (!m->sh_num_shards || !m->sh_num_replicas) {
		eprintk("num_shards = %d num_replicas = %d",
			m->sh_num_shards, m->sh_num_replicas);
		goto error;
	}
	tau_shard.ts_shard_no       = m->sh_shard_no;
	tau_shard.ts_replica_no     = m->sh_replica_no;
	tau_shard.ts_next_ino       = m->sh_shard_no;
	tau_shard.ts_root           = 0;
	while (tau_shard.ts_next_ino <= TAU_SUPER_INO) {
		tau_shard.ts_next_ino += m->sh_num_shards;
	}
	rc = tau_insert_shard( &bag->bag_tree, &tau_shard);
	if (rc) {
		eprintk("tau_insert_shard %d", rc);
		goto error;
	}
	rc = send_tau(replykey, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		goto error;
	}
	return;
error:
	destroy_key_tau(replykey);
}

static void read_bag (void *msg)
{
	fsmsg_s		*m       = msg;
	ki_t		replykey = m->q.q_passed_key;
	bag_s		*bag     = m->q.q_tag;
	struct inode	*isuper  = bag->bag_isuper;
	tau_bag_s	*tbag    = bag->bag_block;
	void		*data;
	int		rc;
FN;
	if (m->io_blkno >= tbag->tb_numblocks) {
		eprintk("blockno too big %llu>=%llu",
			m->io_blkno, tbag->tb_numblocks);
		goto error;
	}
	data = tau_bget(isuper->i_mapping, m->io_blkno);
	if (!data) {
		eprintk("couldn't read block %llu", m->io_blkno);
		goto error;
	}
	rc = write_data_tau(replykey, PAGE_SIZE, data, 0);
	tau_bput(data);
	if (rc) {
		eprintk("write_data_tau %d", rc);
		goto error;
	}
	m->bag_state      = tbag->tb_state;
	m->bag_num_blocks = tbag->tb_numblocks;
	rc = send_tau(replykey, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		goto error;
	}
	return;
error:
	destroy_key_tau(replykey);
}

static void write_bag (void *msg)
{
	fsmsg_s		*m       = msg;
	ki_t		replykey = m->q.q_passed_key;
	bag_s		*b       = m->q.q_tag;
	struct inode	*isuper  = b->bag_isuper;
	tau_bag_s	*tbag    = b->bag_block;
	void		*data    = NULL;
	int		rc;
FN;
	if (m->io_blkno >= tbag->tb_numblocks) {
		eprintk("blockno too big %llu>=%llu",
			m->io_blkno, tbag->tb_numblocks);
		goto error;
	}
	data = tau_bget(isuper->i_mapping, m->io_blkno);
	if (!data) {
		eprintk("couldn't read block %llu", m->io_blkno);
		goto error;
	}
	rc = read_data_tau(replykey, PAGE_SIZE, data, 0);
	if (rc) {
		tau_bput(data);
		eprintk("read_data_tau %d", rc);
		goto error;
	}
	tau_bflush(data);
	tau_bput(data);
	m->bag_state      = tbag->tb_state;
	m->bag_num_blocks = tbag->tb_numblocks;
	rc = send_tau(replykey, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		goto error;
	}
	return;
error:
	destroy_key_tau(replykey);
}

static void finish_bag (void *msg)
{
	fsmsg_s		*m       = msg;
	ki_t		replykey = m->q.q_passed_key;
	bag_s		*bag     = m->q.q_tag;
	tau_bag_s	*tbag    = bag->bag_block;
	avatar_s	*save_av = peek_avatar();
	int		rc;
FN;
	tbag->tb_state        = m->vol_state;
	tbag->tb_num_bags     = m->vol_num_bags;
	tbag->tb_num_shards   = m->vol_num_shards;
	tbag->tb_num_replicas = m->vol_num_replicas;
	tau_bflush(tbag);
	init_shards(bag);
	enter_tau(save_av);
	rc = send_tau(replykey, m);
	if (rc) {
		eprintk("send_tau %d", rc);
		goto error;
	}
	exit_tau();
	return;
error:
	destroy_key_tau(replykey);
	exit_tau();
}

static void ino_open (void *msg)
{
	eprintk("Not Implemented");
#if 0
	fsmsg_s			*m = msg;
	bag_s		*bag = m->q.q_tag;
	struct inode		*isuper = bag->bag_isuper;
	tau_inode_s		*iraw = NULL;

	ki_t			reply_key = m->q.q_passed_key;
	file_s			*file;
	ki_t			key = 0;
	int			rc;
FN;
	file = zalloc_tau(sizeof(file_s));
	if (!file) goto error;

	file->f_type = &File_type.ft_tag;
	file->f_bag  = bag;

	iraw = tau_bget(isuper->i_mapping, m->fs_inode.fs_ino);
	if (!iraw) {
		eprintk("tau_bget err reading inode=%lld",
				m->fs_inode.fs_ino);
		goto error;
	}
	if ((iraw->t_ino != m->fs_inode.fs_ino)
	 && (iraw->h_magic != TAU_MAGIC_INODE))
	{
		eprintk("corrupt inode=%lld\n", m->fs_inode.fs_ino);
		goto error;
	}
	file->f_raw = *iraw;
	init_tree( &file->f_tree, &Tau_dir_species,
			isuper, iraw->t_btree_root);

	key = make_gate(file, RESOURCE | PASS_REPLY);
	if (!key) goto error;

	m->q.q_passed_key = key;
	m->fs_inode.fs_ino    = iraw->t_ino;
	m->fs_inode.fs_size   = iraw->t_size;
	m->fs_inode.fs_blocks = iraw->t_blocks;
	m->fs_inode.fs_mode   = iraw->t_mode;
	m->fs_inode.fs_uid    = iraw->t_uid;
	m->fs_inode.fs_gid    = iraw->t_gid;
	m->fs_inode.fs_atime  = iraw->t_atime.tv_sec;
	m->fs_inode.fs_mtime  = iraw->t_mtime.tv_sec;
	m->fs_inode.fs_ctime  = iraw->t_ctime.tv_sec;
	tau_bput(iraw); iraw = NULL;
	rc = send_key_tau(reply_key, m);
	if (rc) {
		eprintk("send_key_tau %d", rc);
		goto error;
	}
	return;
error:
	destroy_key_tau(reply_key);
	if (key) {
		destroy_key_tau(key);
	} else {
		if (file) free_tau(file);
		if (iraw) tau_bput(iraw);

	}
#endif
}

static void ino_lookup (void *msg)
{
	eprintk("Not Implemented");
}

struct bag_type_s {
	type_s		bt_tag;
	method_f	bt_ops[BAG_OPS];
} Bag_type = { { "Bag", BAG_OPS, 0 },
		{ stat_bag,
		  read_bag,
		  write_bag,
		  get_shard,
		  put_shard,
		  finish_bag,
		  ino_open,
		  ino_lookup} };
	
static int post_bag (bag_s *bag)
{
	ki_t	key = 0;
	int	rc = 0;
FN;

	bag->bag_type = &Bag_type.bt_tag;

	key = make_gate(bag, REQUEST | PASS_REPLY);
	if (!key) goto error;

	rc = sw_post("bag", key);
	if (rc) goto error;

	return 0;
error:
	eprintk("failed initialization for bag %d", rc);
	if (key) {
		destroy_key_tau(key);
	}
	return rc;
}

static void bag_set_root_inode (struct inode *inode)
{
	struct super_block	*sb = inode->i_sb;

FN;
	inode->i_blkbits = sb->s_blocksize_bits;
	inode->i_blksize = sb->s_blocksize;
	inode->i_rdev    = sb->s_dev;
	inode->i_mode    = S_IFDIR;
	inode->i_uid     = 0;
	inode->i_gid     = 0;
	inode->i_atime   = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_size    = 0;
	inode->i_blocks  = 0;
	inode->i_op      = &bag_inode_ops;
	inode->i_fop     = &bag_dir_ops;
	inode->i_mapping->a_ops = &bag_address_ops;
}

static void bag_put_super (struct super_block *sb)
{
	bag_s		*bag = sb->s_fs_info;
	struct inode	*isuper = bag->bag_isuper;
FN;
PRinode(isuper);
	free_shards(bag);
	tau_bput(bag->bag_block);
	truncate_inode_pages( &isuper->i_data, 0);
	sb->s_fs_info = NULL;
	kfree(bag);	
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,16))
static int bag_statfs (struct dentry *dentry, struct kstatfs *statfs)
{
	struct super_block *sb = dentry->d_sb;
#elif ((LINUX_VERSION_CODE == KERNEL_VERSION(2,6,16)) && (EXTRAVERSION > 56))
static int bag_statfs (struct dentry *dentry, struct kstatfs *statfs)
{
	struct super_block *sb = dentry->d_sb;
#else
static int bag_statfs (struct super_block *sb, struct kstatfs *statfs)
{
#endif
	bag_s		*bag  = sb->s_fs_info;
	tau_bag_s	*tbag = bag->bag_block;
FN;
	statfs->f_type    = sb->s_magic;
	statfs->f_bsize   = sb->s_blocksize;
	statfs->f_blocks  = tbag->tb_numblocks;
	statfs->f_bfree   = tbag->tb_numblocks - tbag->tb_nextblock;
	statfs->f_bavail  = statfs->f_bfree;
	statfs->f_files   = 0;
	statfs->f_ffree   = 0;
	statfs->f_fsid.val[0] = sb->s_dev;	// See comments in "man statfs"
	statfs->f_fsid.val[1] = 0;
	statfs->f_namelen = TAU_FILE_NAME_LEN;
	statfs->f_frsize  = 0;	// Don't know what this is for.
	return  0;
}

static struct super_operations tau_sops = {
	.read_inode	= bag_set_root_inode,
	.put_super	= bag_put_super,
//	.write_super	= bag_write_super,
//	.sync_fs	= bag_sync_fs,
	.statfs		= bag_statfs,
//	.remount_fs	= bag_remount,
};

static int tau_fill_super (
	struct super_block	*sb,
	void			*data,		/* Command line */
	int			isSilent)
{
	struct block_device	*dev = sb->s_bdev;
	struct inode	*isuper = NULL;
	bag_s		*bag    = NULL;
	tau_bag_s	*tbag   = NULL;
	unint		sectorSize;
	int		rc = 0;
FN;
	enter_tau(Fs_avatar);
	sectorSize = sb_min_blocksize(sb, BLOCK_SIZE);
	if (sectorSize > TAU_BLK_SIZE) {
		// Not going to deal with mongo sectors today
		rc = -EMEDIUMTYPE;
		goto error;
	}
	set_blocksize(dev, TAU_BLK_SIZE);

	sb->s_blocksize	     = TAU_BLK_SIZE;
	sb->s_blocksize_bits = TAU_BLK_SHIFT;
	sb->s_maxbytes	     = TAU_MAX_FILE_SIZE;
	sb->s_magic	     = (__u32)TAU_MAGIC_SUPER;
	sb->s_op             = &tau_sops;
	sb->s_export_op      = NULL;

	bag = kmalloc(sizeof(bag_s), GFP_KERNEL);
	if (!bag) {
		rc = -ENOMEM;
		goto error;
	}
	bag->bag_super  = sb;
	sb->s_fs_info = bag;

	isuper = iget(sb, TAU_SUPER_INO);	//XXX: check error
	bag->bag_isuper = isuper;

	tbag = tau_bget(isuper->i_mapping, TAU_SUPER_BLK);
	if (!tbag) {
		eprintk("couldn't read super block");
		goto error;
	}
	// XXX: need to make sure page gets cleaned up on error
	if (tbag->h_magic != TAU_MAGIC_SUPER) {
		printk(KERN_ERR "tau: bad magic %u!=%u\n",
					TAU_MAGIC_SUPER,
					tbag->h_magic);
		goto error;
	}
	bag->bag_block = tbag;	// Need to save page - unmap it ?

	tau_loginit( &bag->bag_log);

	sb->s_root = d_alloc_root(isuper);
	if (!sb->s_root) {
		eprintk("get root inode failed\n");
		goto error;
	}
	init_tree( &bag->bag_tree, &Tau_shard_species,
			isuper, tbag->tb_shard_btree);
	rc = post_bag(bag);
	if (rc) goto error;
exit:
	exit_tau();
	return rc;

error:
	if (rc == 0) rc = -EIO;
	if (tbag) tau_bput(tbag);
	if (sb->s_root) {
		dput(sb->s_root);
		sb->s_root = NULL;
	} else if (isuper) {
		iput(isuper);
	}
	if (bag) {
		kfree(bag);
		sb->s_fs_info = NULL;
	}
	goto exit;
}

static struct super_block *tau_get_sb (
	struct file_system_type	*fs_type,
	int			flags,
	const char		*dev_name,
	void			*data)
{
FN;
	return  get_sb_bdev(fs_type, flags, dev_name, data, tau_fill_super);
}

static struct file_system_type tau_fs_type = {
	.owner    = THIS_MODULE,
	.name     = "tau",
	.get_sb   = tau_get_sb,
	.kill_sb  = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
};

static void fs_receive (int rc, void *msg)
{
	struct object_s {
		type_s	*obj_type;
	} *obj;
	msg_s	*m = msg;
	type_s	*type;
FN;
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

static int start_msg (void)
{
	int	rc;
FN;
	Fs_avatar = init_msg_tau("taufs", fs_receive);
	if (!Fs_avatar) return -1;

	rc = sw_register("taufs");
	exit_tau();
	if (rc) return rc;

	return 0;
}

static void stop_msg (void)
{
FN;
	if (Fs_avatar) {
		die_tau(Fs_avatar);
		Fs_avatar = NULL;
	}
}

void tau_exit (void)
{
	unregister_filesystem( &tau_fs_type);
	stop_msg();
	tau_stop_log();
	destroy_tau_buf_cache();
}

int tau_init (void)
{
	int	rc;

FN;
	rc = init_tau_buf_cache();
	if (rc) goto error;

	rc = tau_start_log();
	if (rc) goto error;

	rc = start_msg();
	if (rc) goto error;

	rc = register_filesystem( &tau_fs_type);
	if (rc) goto error;

	return 0;
error:
	printk(KERN_INFO "tau error=%d\n", rc);
	tau_exit();
	return rc;	
}
