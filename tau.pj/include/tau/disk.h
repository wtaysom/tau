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

#ifndef _TAU_DISK_H_
#define _TAU_DISK_H_ 1

#ifndef _STYLE_H
#include <style.h>
#endif

enum tau_super_state_t {
	TAU_BAG_INVALID = 0,
	TAU_BAG_NEW,
	TAU_BAG_VOLUME,
	TAU_BAG_DIEING};

enum {
	TAU_BLK_SHIFT        = 12,	// 4096 byte block
	TAU_BLK_SIZE         = (1 << TAU_BLK_SHIFT),
	TAU_OFFSET_MASK      = (TAU_BLK_SIZE - 1),
	TAU_BITS_PER_BLK     = TAU_BLK_SIZE * 8,

	TAU_MAX_REPLICAS     = 10,

	TAU_STAGE_SIZE       = 32,	// Num blocks in a log stage
	TAU_STAGES           = 4,	// Num stages in log

	TAU_ROOT_INO         = 1,
	TAU_SUPER_BLK        = 2,
	TAU_SUPER_INO        = 2,
	TAU_START_LOG        = 1 + TAU_SUPER_BLK,
	TAU_START_FREE       = TAU_START_LOG + (TAU_STAGE_SIZE * TAU_STAGES),

	TAU_INOS_PER_BLK     = (TAU_BLK_SIZE / sizeof(__u64)),
	TAU_NUM_EXTENTS      = (1 << 4),
	TAU_ALU              = (1ULL << 8),
	TAU_MAX_FILE_SIZE    = TAU_NUM_EXTENTS * TAU_ALU * TAU_BLK_SIZE,
	TAU_CLEAR_ENDS       = 16,	// Num blocks cleared at ends of fs
	TAU_MIN_BLOCKS       = TAU_CLEAR_ENDS,	// Min fs size

	TAU_SUPER_VERSION    = 17,
	TAU_INO_VERSION      = 23,
	TAU_DIR_VERSION      = 13,
	TAU_MAGIC_SUPER      = 0xba6ba6,
	TAU_MAGIC_INODE      = 0xdeed,
	TAU_MAGIC_DIR        = 0xbace,
	KACHE_MAGIC_SUPER    = 0xbabe,

	TAU_FILE_NAME_LEN	= 255,	// *3?
	TAU_DOT		= -2,
	TAU_DOTDOT	= -1
};

#define TAU_SUPER_HEAD(_sb, _blknum) {		\
	(_sb)->h_blknum  = _blknum;		\
	(_sb)->h_stage   = 0;			\
	(_sb)->h_magic   = TAU_MAGIC_SUPER;	\
	(_sb)->h_version = TAU_SUPER_VERSION;	\
}

#define TAU_INODE_HEAD(_ino, _blknum) {		\
	(_ino)->h_blknum  = _blknum;		\
	(_ino)->h_stage   = 0;			\
	(_ino)->h_magic   = TAU_MAGIC_INODE;	\
	(_ino)->h_version = TAU_INO_VERSION;	\
}

#define TAU_DIR_HEAD(_dir, _blknum) {		\
	(_dir)->h_blknum  = _blknum;		\
	(_dir)->h_stage   = 0;			\
	(_dir)->h_magic   = TAU_MAGIC_DIR;	\
	(_dir)->h_version = TAU_DIR_VERSION;	\
}

/*
 * Anonymous structure for the header of each meta-data block
 */
#define HEAD_S struct {		\
	__u64	h_blknum;	\
	__u64	h_stage;	\
	__u32	h_magic;	\
	__u32	h_version;	\
}

typedef struct head_s {
	HEAD_S;
} head_s;

typedef struct tau_bag_s {
	HEAD_S;
	guid_t		tb_guid_vol;
	guid_t		tb_guid_bag;

	__u32		tb_state;
	__u32		tb_num_bags;
	__u32		tb_num_shards;
	__u32		tb_num_replicas;

	__u64		tb_numblocks;
	__u64		tb_nextblock;
	__u64		tb_log_start;
	__u64		tb_log_size;

	__u64		tb_shard_btree;
} tau_bag_s;

typedef struct tau_shard_s {
	__u32		ts_shard_no;
	__u32		ts_replica_no;
	__u64		ts_next_ino;
	__u64		ts_root;
} tau_shard_s;

typedef struct tau_extent_s {
	__u64	e_start;
	__s64	e_length;	/* Length of extent in blocks, negative
				 * lengths are for holes
				 */
} tau_extent_s;

typedef struct tau_inode_s {
	__u64		t_ino;
	__u64		t_parent;	/* Parent inode */
	__u16		t_mode;
	__u16		t_link;		/* Reserved for future Evil */
	__u32		t_blksize;
	__u64		t_size;		/* Logical size of file (EOF) */
	__u64		t_blocks;	/* Total blocks assigned to file */
	__u32		t_uid;
	__u32		t_gid;
	struct timespec	t_atime;
	struct timespec	t_mtime;
	struct timespec	t_ctime;
	__u64		t_btree_root;
	__u16		t_name_len;
	__u16		t_reserved[3];
	tau_extent_s	t_extent[TAU_NUM_EXTENTS];
	char		t_name[0];
} tau_inode_s;

#endif
