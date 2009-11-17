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

#ifndef _PDISK_H_
#define _PDISK_H_ 1

typedef struct pi_head_s {
	__u64	h_magic;
	__u64	h_seqno;
	__u32	h_version;
	__u32	h_reserved;
} pi_head_s;

#define PI_SUPER_HEAD(_sb) {				\
	(_sb)->sb_hd.h_magic   = PI_MAGIC_SUPER;	\
	(_sb)->sb_hd.h_seqno   = 1;			\
	(_sb)->sb_hd.h_version = PI_SUPER_VERSION;	\
}

#define PI_INODE_HEAD(_ino) {				\
	(_ino)->t_hd.h_magic   = PI_MAGIC_INODE;	\
	(_ino)->t_hd.h_seqno   = 1;			\
	(_ino)->t_hd.h_version = PI_INO_VERSION;	\
}

#define PI_DIR_HEAD(_dir) {				\
	(_dir)->d_hd.h_magic   = PI_MAGIC_DIR;	\
	(_dir)->d_hd.h_seqno   = 1;			\
	(_dir)->d_hd.h_version = PI_DIR_VERSION;	\
}

typedef struct pi_entry_s {
	__u64	e_ino;
	__u32	e_hash;
	__u32	e_reserved;
} pi_entry_s;

enum {
	PI_SECTOR_SIZE     = 512,
	PI_BLK_SHIFT       = 12,	/* 4096 byte block */
	PI_BLK_SIZE        = (1 << PI_BLK_SHIFT),
	PI_OFFSET_MASK     = (PI_BLK_SIZE - 1),
	PI_SECTORS_PER_BLK = (PI_BLK_SIZE / PI_SECTOR_SIZE),
	PI_BITS_PER_BLK    = 8 * PI_BLK_SIZE,

	PI_SUPER_BLK       = 1,
	PI_SUPER_INO       = 1,
	PI_ROOT_INO        = 1 + PI_SUPER_INO,
	PI_START_FREE      = 1 + PI_ROOT_INO,

	PI_NAME_LEN        = 255,	// *3?

	PI_INOS_PER_BLK    = (PI_BLK_SIZE / sizeof(u64)),
	PI_INDIRECT_SHIFT  = 8,
	PI_NUM_EXTENTS     = (1 << 6),
	PI_ALU             = (1ULL << 8),
	PI_MAX_LEVELS      = 6,
	PI_ENTRIES_PER_BLK = ((PI_BLK_SIZE - sizeof(pi_head_s))
					/ sizeof(pi_entry_s)),
	PI_MAX_FILE_SIZE   = PI_NUM_EXTENTS * PI_ALU * PI_BLK_SIZE,
	PI_CLEAR_ENDS      = 16,	// Num blocks cleared at ends of fs
	PI_MIN_BLOCKS      = PI_CLEAR_ENDS,	// Min fs size

	PI_SUPER_VERSION   = 1,
	PI_INO_VERSION     = 1,
	PI_DIR_VERSION     = 1,
	PI_MAGIC_SUPER     = 0xbabe,
	PI_MAGIC_INODE     = 0xdead,
	PI_MAGIC_DIR       = 0xface,
};

typedef struct pi_super_block_s
{
	pi_head_s	sb_hd;
	__u64		sb_numblocks;
	__u64		sb_nextblock;
	__u32		sb_blocksize;
} pi_super_block_s;

typedef struct pi_extent_s {
	__u64	e_start;
	__u64	e_length;	/* Length of extent in blocks */
} pi_extent_s;

typedef struct pi_inode_s {
	pi_head_s	t_hd;
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
	char		t_name[PI_NAME_LEN+1];
	pi_extent_s	t_extent[PI_NUM_EXTENTS];
} pi_inode_s;

typedef struct pi_dir_s {
	pi_head_s	d_hd;
	pi_entry_s	d_entry[PI_ENTRIES_PER_BLK];
} pi_dir_s;

typedef struct pi_blk_s {
	__u64	b_inode;
	__u64	b_logical;
} pi_blk_s;


#endif
