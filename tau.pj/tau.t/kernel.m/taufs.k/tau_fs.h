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

#ifndef _TAU_FS_H_
#define _TAU_FS_H_ 1

#ifndef _Q_H_
#include <q.h>
#endif

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

#ifndef _TAU_TAG_H_
#include <tau/tag.h>
#endif

#ifndef _TAU_DISK_H_
#include <tau/disk.h>
#endif

#ifndef _TAU_BLK_H_
#include <tau_blk.h>
#endif

#ifndef _TAU_BTREE_H_
#include <tau_btree.h>
#endif

typedef struct shard_s	shard_s;
typedef struct hub_s	hub_s;

typedef struct tau_spinlock_s {
	spinlock_t		lk_spinlock;
	unsigned long		lk_flags;
	struct task_struct	*lk_owner;
	const char		*lk_where;
	const char		*lk_name;
} tau_spinlock_s;

enum { STAGE_IDLE = 0, STAGE_FILLING, STAGE_FLUSHING, STAGE_HOMING };

typedef struct tau_stage_s {
	u64			stg_num;
	u64			stg_blknum;
	tau_spinlock_s		stg_lock;
	struct semaphore	stg_flush_mutex;
	struct semaphore	stg_home_mutex;
	struct work_struct	stg_work;
//	unint			stg_state;
	unint			stg_numpages;
	struct page		*stg_page[TAU_STAGE_SIZE];
} tau_stage_s;

typedef struct tau_log_s {
	tau_spinlock_s		lg_lock;
	struct semaphore	lg_stage_mutex;
	struct semaphore	lg_user_mutex;
	bool			lg_staging;
	unint			lg_users;
	tau_stage_s		*lg_current;
	tau_stage_s		lg_stage[TAU_STAGES];
} tau_log_s;

typedef struct bag_s {
	type_s			*bag_type;
	u64			bag_instance;
	struct super_block	*bag_super;
	struct inode		*bag_isuper;
	tau_bag_s		*bag_block;
	tau_log_s		bag_log;
	tree_s			bag_tree;
} bag_s;

typedef struct spoke_s {
	type_s		*ri_type;
	hub_s		*ri_hub;
	dqlink_t	ri_link;
	ki_t		ri_key;
} spoke_s;

struct hub_s {
	hub_s		*h_hash;
	shard_s		*h_shard;
	d_q		h_spokes;
	tree_s		h_dir;
	u64		h_dirty : 1;
	tau_inode_s	h_tnode;
};

enum { HUB_BUCKETS = 107 };

struct shard_s {
	type_s		*sh_type;
	avatar_s	*sh_avatar;
	dqlink_t	sh_list;
	guid_t		sh_vol_id;
	u64		sh_inuse;
	tau_shard_s	sh_disk;
	bag_s		*sh_bag;
	tree_s		sh_inodes;
	hub_s		*sh_hub_buckets[HUB_BUCKETS];
};

typedef struct shard_name_s {
	guid_t	sn_guid;
	u32	sn_shard_no;
	u32	sn_replica_no;
} shard_name_s;

typedef struct file_s {
	type_s		*f_type;
	bag_s		*f_bag;
	tree_s		f_tree;
} file_s;
	
extern avatar_s	*Fs_avatar;

extern tree_species_s	Tau_dir_species;
extern tree_species_s	Tau_shard_species;
extern tree_species_s	Tau_hub_species;

int  tau_fill_page   (struct page *page, u64 pblk);
int  tau_flush_page  (struct page *page);
void tau_flush_stage (tau_stage_s *stage);
void tau_home_stage  (tau_stage_s *stage);

#define PRpage(_x_)	tau_pr_page(__func__, __LINE__, _x_)

void tau_pr_page  (const char *fn, unsigned line, struct page *p);

u64  tau_alloc_block (struct super_block *sb, unint num_blocks);

int tau_delete_dir (tree_s *ptree, const u8 *name);
int tau_insert_dir (tree_s *ptree, u64 ino, const u8 *name);
u64 tau_find_dir   (tree_s *ptree, const u8 *name);
int tau_next_dir   (tree_s *ptree,
			u64	key,
			u64	*next_key,
			u8	*name,
			u64	*ino);

int tau_delete_shard (tree_s *ptree, tau_shard_s *tau_shard);
int tau_insert_shard (tree_s *ptree, tau_shard_s *tau_shard);
int tau_find_shard   (tree_s *ptree, tau_shard_s *tau_shard);

void tau_loginit  (tau_log_s *log);
void tau_logbegin (struct inode *inode);
void tau_logend   (struct inode *inode);
void tau_log      (struct page *page);

int  tau_start_log (void);
void tau_stop_log  (void);

void tau_init_lock      (tau_spinlock_s *lock, const char *name);
void tau_lock           (tau_spinlock_s *lock, const char *where);
void tau_unlock         (tau_spinlock_s *lock, const char *where);
void tau_have_lock      (tau_spinlock_s *lock, const char *where);
void tau_dont_have_lock (tau_spinlock_s *lock, const char *where);

void tau_fill (struct page *page, u64 blknum);
int  tau_readpage (struct file *file, struct page *page);
int  tau_invalidatepage (struct page *page, unsigned long offset);


int init_tau_buf_cache     (void);
void destroy_tau_buf_cache (void);

void     init_shards (bag_s *bag);
void     free_shards (bag_s *bag);
shard_s *new_shard   (bag_s *bag, tau_shard_s *tau_shard);


hub_s *make_root_hub (shard_s *shard);

int tau_find_hub   (tree_s *ptree, u64 ino, hub_s **phub);
int tau_insert_hub (tree_s *ptree, hub_s *hub);
int tau_delete_hub (tree_s *ptree, u64 ino);

hub_s *open_hub (shard_s *shard, u64 ino);
hub_s *alloc_hub (unint name_len);
void   free_hub (hub_s *hub);


static inline bool uuid_is_equal(const guid_t a, const guid_t b)
{
	return (memcmp(a, b, sizeof(guid_t)) == 0);
}

static inline int uuid_compare(const guid_t a, const guid_t b)
{
	return memcmp(a, b, sizeof(guid_t));
}

static inline void uuid_copy(guid_t dst, const guid_t src)
{
	memcpy(dst, src, sizeof(guid_t));
}


#endif
