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

#ifndef _TAU_FSMSG_H_
#define _TAU_FSMSG_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

#ifndef PAGE_SIZE
#include <asm/page.h>
#endif

enum {	STAT_BAG = 0,
	READ_BAG,
	WRITE_BAG,
	GET_SHARD,
	PUT_SHARD,
	FINISH_BAG,
	INO_OPEN,
	INO_LOOKUP,
	BAG_OPS };

enum {	INODE_OPEN = 0,	//XXX: take INO_OPEN and INO_LOOKUP
	CREATE_SPOKE,	//XXX: follow this pattern for names
	SHARD_OPS };

enum {	REGISTER_OPEN = 0,
	ADD_DIR_ENTRY,
	LOOKUP_DIR_ENTRY,
	PREPARE_WRITE,
	FLUSH_PAGE,
	SPOKE_OPS };

enum {	ZERO_FILL = 0,
	DATA_FILL = 1 };

enum {	INO_WRITE = 0,
	FILE_READ,
	FILE_WRITE,
	FILE_READDIR,
	FILE_OPS };

enum {	UP_SIZE   = 0x1,
	UP_BLOCKS = 0x2,
	UP_MODE   = 0x4,
	UP_UID    = 0x8,
	UP_GID    = 0x10,
	UP_ATIME  = 0x20,
	UP_MTIME  = 0x40,
	UP_CTIME  = 0x80,
	UP_PARENT = 0x100,
	UP_NAME   = 0x200};

typedef struct fs_inode_s {
	u64	fs_ino;     /* inode number */
	u64	fs_size;    /* total size, in bytes */
	u64	fs_blocks;  /* number of blocks allocated */
	u64	fs_seqno;   /* sequence number */
	u32	fs_mode;    /* type and protection */
	u32	fs_uid;     /* user ID of owner */
	u32	fs_gid;     /* group ID of owner */
	u32	fs_atime;   /* time of last access */
	u32	fs_mtime;   /* time of last modification */
	u32	fs_ctime;   /* time of last status change */
} fs_inode_s;

typedef struct fsmsg_s {
	sys_s	q;
	union {
		METHOD_S;
		body_u	b;
		struct {
			METHOD_S;
			char	lk_name[TAU_NAME];
		};
		struct {
			METHOD_S;
			u64	rw_offset;
			u64	rw_length;
		};
		struct {
			METHOD_S;
			u64	rd_cookie;
		};
		struct {
			METHOD_S;
			fs_inode_s	fs_inode;
		};
		struct {
			u8		up_method;
			u8		up_reserved[3];
			u32		up_flags;
			fs_inode_s	up_inode;
		};
		struct {			// Create crt
			METHOD_S;
			u64	crt_parent;	// Parent inode number
			u64	crt_child;	// Child inode number
			u32	crt_mode;	// File type
			u32	crt_uid;	// Who is creating this thing
			u32	crt_gid;	// Group for who
		};
		struct {			// Create reply
			METHOD_S;
			u64	crr_ino;
		};
		struct {
			METHOD_S;
			u64	ino_start;
			u64	ino_end;
		};
		struct {
			METHOD_S;
			char	fm_name[TAU_NAME];
			guid_t	fm_guid;
		};
		struct {
			METHOD_S;
			guid_t	bag_guid_bag;
			guid_t	bag_guid_vol;
			u64	bag_state;
			u64	bag_num_blocks;
			u32	bag_num_shards;
			u32	bag_num_replicas;
		};
		struct {
			METHOD_S;
			u32	sh_shard_no;
			u32	sh_replica_no;
			u32	sh_num_shards;
			u32	sh_num_replicas;
		};
		struct {
			METHOD_S;
			guid_t	vol_guid;
			u32	vol_state;
			u32	vol_num_bags;
			u32	vol_num_shards;
			u32	vol_num_replicas;
		};
		struct {
			METHOD_S;
			u64	io_blkno;
		};
	};
} fsmsg_s;

enum { DIR_LIST_LIMIT = PAGE_SIZE - sizeof(u16)};

typedef struct dir_list_s {
	u16	dl_next;
	char	dl_list[DIR_LIST_LIMIT];
} dir_list_s;

#define PACK(_dest, _field, _i)	{				\
	memcpy( &(_dest)[(_i)], &(_field), sizeof(_field));	\
	(_i) += sizeof(_field);					\
}

#define UNPACK(_field, _src, _i) {				\
	memcpy( &(_field),  &(_src)[(_i)], sizeof(_field));	\
	(_i) += sizeof(_field);					\
}

enum check_sizes {
	MSG_SIZE_CK = 1 / (sizeof(fsmsg_s) == sizeof(msg_s)),
	DIR_LIST_LIMIT_CK = 1 / (sizeof(dir_list_s) == PAGE_SIZE)};

#endif
