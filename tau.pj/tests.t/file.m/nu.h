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

#ifndef _ZOMNI_H_
#include <zOmni.h>
#endif

enum {
	NU_SECTOR_SIZE		= 512,
	NU_BLK_SHIFT		= 12,		/* 4096 byte block */
	NU_BLK_SIZE			= (1 << NU_BLK_SHIFT),
	NU_SECTORS_PER_BLK	= (NU_BLK_SIZE / NU_SECTOR_SIZE),
	NU_NUM_BLKS			= 1000,
	NU_FS_SIZE			= ((long long)NU_NUM_BLKS) * NU_BLK_SIZE,
	NU_SUPER_BLK		= 0,
	NU_ROOT_INO			= 1,
	NU_FREE_INO			= 2,
	NU_FREE_BLKS		= 3,
	NU_NAME_LEN			= 255,
	NU_INOS_PER_BLK		= (NU_BLK_SIZE / sizeof(QUAD)),
	NU_NUM_DIRECT		= 20,
	NU_MAX_FILE_SIZE	= NU_NUM_DIRECT * NU_BLK_SIZE,
	NU_INO_VERSION		= 1,	/* Increment when on disk inode format changes */	
	NU_MAGIC_SB			= 0x7265707573756174LLU,
	NU_MAGIC_NODE		= 0x65646f6e69756174LLU
	
};

typedef struct Inode_s	Inode_s;
typedef union Blk_u		Blk_u;

typedef struct SuperBlock_s
{
	QUAD	sb_magic;
	QUAD	sb_seqno;
	QUAD	sb_numBlocks;
	LONG	sb_blkSize;
} SuperBlock_s;


struct Inode_s
{
	QUAD		t_magic;
	QUAD		t_seqno;	/* Sequence number */
	QUAD		t_ino;
	QUAD		t_parent;	/* Parent inode */
	LONG		t_version;	/* Version number of Tnode - for upgrades */
	LONG		t_reserved;
	WORD		t_mode;
	WORD		t_link;		/* Necessary Evil */
	LONG		t_blksize;
	QUAD		t_size;		/* Logical size of file (EOF) */
	QUAD		t_blocks;	/* Total blocks assigned to file */
	uid_t		t_uid;
	gid_t		t_gid;
	QUAD		t_atime;
	QUAD		t_mtime;
	QUAD		t_ctime;
	QUAD		t_direct[NU_NUM_DIRECT];
	char		t_name[NU_NAME_LEN+1];
};

union Blk_u
{
	Blk_u			*b_next;
	char			b_data[NU_BLK_SIZE];
	SuperBlock_s	b_super;
	Inode_s			b_inode;
	QUAD			b_ino[NU_INOS_PER_BLK];
	NINT			b_bitmap[NU_BLK_SIZE / sizeof(NINT)];
};

#endif
