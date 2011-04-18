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

#ifndef _NU_H_
#define _NU_H_

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _STYLE_H_
#include <style.h>
#endif

enum {
	NU_SECTOR_SIZE		= 512,
	NU_BLK_SHIFT		= 12,		/* 4096 byte block */
	NU_BLK_SIZE		= (1 << NU_BLK_SHIFT),
	NU_SECTORS_PER_BLK	= (NU_BLK_SIZE / NU_SECTOR_SIZE),
	NU_NUM_BLKS		= 1000,
	NU_FS_SIZE		= ((s64)NU_NUM_BLKS) * NU_BLK_SIZE,
	NU_SUPER_BLK		= 0,
	NU_ROOT_INO		= 1,
	NU_FREE_INO		= 2,
	NU_FREE_BLKS		= 3,
	NU_NAME_LEN		= 255,
	NU_INOS_PER_BLK		= (NU_BLK_SIZE / sizeof(u64)),
	NU_NUM_DIRECT		= 20,
	NU_MAX_FILE_SIZE	= NU_NUM_DIRECT * NU_BLK_SIZE,
	NU_INO_VERSION		= 1,	/* Increment when on disk inode format changes */	
	NU_MAGIC_SB		= 0x7265707573756174LLU,
	NU_MAGIC_NODE		= 0x65646f6e69756174LLU
	
};

typedef struct Inode_s	Inode_s;
typedef union Blk_u	Blk_u;

typedef struct SuperBlock_s
{
	u64	sb_magic;
	u64	sb_seqno;
	u64	sb_numBlocks;
	u32	sb_blkSize;
} SuperBlock_s;


struct Inode_s
{
	u64	t_magic;
	u64	t_seqno;	/* Sequence number */
	u64	t_ino;
	u64	t_parent;	/* Parent inode */
	u32	t_version;	/* Version number of Tnode - for upgrades */
	u32	t_reserved;
	u16	t_mode;
	u16	t_link;		/* Necessary Evil */
	u32	t_blksize;
	u64	t_size;		/* Logical size of file (EOF) */
	u64	t_blocks;	/* Total blocks assigned to file */
	uid_t	t_uid;
	gid_t	t_gid;
	u64	t_atime;
	u64	t_mtime;
	u64	t_ctime;
	u64	t_direct[NU_NUM_DIRECT];
	char	t_name[NU_NAME_LEN+1];
};

union Blk_u
{
	Blk_u		*b_next;
	char		b_data[NU_BLK_SIZE];
	SuperBlock_s	b_super;
	Inode_s		b_inode;
	u64		b_ino[NU_INOS_PER_BLK];
	unint		b_bitmap[NU_BLK_SIZE / sizeof(unint)];
};

#endif
