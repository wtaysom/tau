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

#ifndef _KACHE_H_
#define _KACHE_H_ 1

#ifndef _LINUX_FS_H
#include <linux/fs.h>
#endif

#ifndef _Q_H_
#include <q.h>
#endif

#ifndef _TAU_DISK_H_
#include <tau/disk.h>
#endif

#ifndef _TAU_BTREE_H_
#include <tau_btree.h>
#endif

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

#ifndef _TAU_TAG_H_
#include <tau/tag.h>
#endif

#ifndef _TAU_FS_H_
#include <tau_fs.h>
#endif

typedef struct knode_s		knode_s;
typedef struct volume_s		volume_s;
typedef struct replica_s	replica_s;

/*
 * For the ext2 file system, there are three structures that share field
 * names with the same name -- inode, ext2_inode, ext2_info_inode.  I
 * would prefer to use names unique to each structure.
 *	inode     - defined across file systems (linux/fs.h).
 *	tau_inode - raw inode on disk format (tau/disk.h)
 *	hub       - physical in memory version of tau_inode (tau_fs.h)
 *	spoke     - hangs off of hub (tau_fs.h)
 *	knode     - logical in memory version of inode (kache.h)
 */
enum {	KN_NORMAL = 1,
	KN_CLOSED,
	KN_DIEING };

//XXX: change ki->kn
struct knode_s {
	type_s		*ki_type;
	replica_s	*ki_replica;	// For avatar, volume, sb
	u64		ki_state;
	tree_s		ki_tree;
	ki_t		ki_key;
	u64		ki_keyid;
	struct inode	ki_vfs_inode;
};

static inline knode_s *get_knode (struct inode *inode)
{
	return container_of(inode, knode_s, ki_vfs_inode);
}

struct replica_s {
	type_s		*rp_sw_type;
	avatar_s	*rp_avatar;
	volume_s	*rp_volume;
	guid_t		rp_vol_id;
	u64		rp_inuse;
	u32		rp_shard_no;
	u32		rp_replica_no;
	ki_t		rp_key;
};

struct volume_s {
	guid_t			vol_guid;
	dqlink_t		vol_list;
	avatar_s		*vol_avatar;
	u64			vol_inuse;
	u64			vol_instance;
	u32			vol_num_shards;
	u32			vol_num_replicas;
	u32			vol_next_replica;
	replica_s		***vol_replicas;
	struct super_block	*vol_sb;
	char			vol_name[TAU_FILE_NAME_LEN+1];
};

int kache_err (int rc);

//XXX:buf_s *kache_assign_buf (struct page *page);
//XXX:void   kache_remove_buf (struct page *page);

int  kache_fill_page   (struct page *page, u64 pblk);
int  kache_flush_page  (struct page *page);

#define PRinode(_x_)	kache_pr_inode(__func__, __LINE__, _x_)

void kache_pr_inode (const char *fn, unsigned line, struct inode *inode);

struct page *kache_get_page (struct inode *inode, u64 blkno);
struct page *kache_new_page (struct inode *dir, u64 blkno);
void kache_put_page (struct page *page);
u64  kache_alloc_block (struct super_block *sb, unint num_blocks);
int  kache_zero_new_page (struct inode *inode, struct page *page);

int kache_delete_dir (tree_s *ptree, const u8 *name);
int kache_insert_dir (tree_s *ptree, u64 ino, const u8 *name);
u64 kache_find_dir   (tree_s *ptree, const u8 *name);
int kache_next_dir   (tree_s *ptree,
			u64	key,
			u64	*next_key,
			u8	*name,
			u64	*ino);

int fill_dir_tree (knode_s *knode);
u64 do_readdir    (unint key, void *dirent, filldir_t filldir, u64 pos);

int uuid_parse (const char *in, guid_t uu);

void kache_dump_tree (tree_s *tree);

int kache_insert (
	tree_s	*tree,
	u64	key,
	void	*rec,
	unint	size);

int kache_delete (
	tree_s	*tree,
	u64	key);

int kache_search (
	tree_s		*tree,
	u64		key,
	search_f	sf,
	void		*data);

void kache_stat_tree (
	tree_s		*tree,
	btree_stat_s	*bs);

int kache_compare_trees (tree_s *a, tree_s *b);


int       add_volume (volume_s *v);
void      rmv_volume (volume_s *v);
volume_s *get_volume (guid_t guid);
void      put_volume (volume_s *v);

int        init_replica_matrix    (volume_s *v);
void       cleanup_replica_matrix (volume_s *v);
replica_s *vol_replica            (volume_s *v, u64 ino);
replica_s *pick_replica           (volume_s *v);

int add_dir_entry (
	knode_s		*parent,
	struct dentry	*dentry,
	u64		ino,
	int		mode);

u64 lookup_dir_entry (
	knode_s		*parent,
	struct dentry	*dentry);

extern tree_species_s  Kache_dir_species;

#endif
