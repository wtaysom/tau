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

#ifndef _PI_H_
#define _PI_H_

#ifndef _PDISK_H_
#include <pdisk.h>
#endif

enum { BUF_MAGIC = 0xace };

typedef struct buf_s {
	struct page	*b_page;
	u64		b_blkno;
	u64		b_magic;
} buf_s;

typedef struct pi_super_info_s
{
	struct inode		*si_isuper;	/* Inode used for physical IO	*/
	struct page		*si_page;	/* Page for superblock		*/
	pi_super_block_s	*si_sb;
} pi_super_info_s;

/*
 * For the ext2 file system, there are three structures that share field
 * names with the same name -- inode, ext2_inode, ext2_info_inode.  I
 * would prefer to use names unique to each structure.
 *	inode - defined across file systems.
 *	pi_inode - raw inode on disk format
 *	pi_info_inode - full in memory version of inode
 */


typedef struct pi_inode_info_s {
	pi_extent_s	ti_extent[PI_NUM_EXTENTS];
	char		ti_name[PI_NAME_LEN+1];
	struct inode	ti_vfs_inode;
} pi_inode_info_s;

static inline pi_inode_info_s *pi_inode (struct inode *inode)
{
	return container_of(inode, pi_inode_info_s, ti_vfs_inode);
}

buf_s *pi_assign_buf (struct page *page);
void   pi_remove_buf (struct page *page);

int pi_fill_page  (struct page *page, u64 pblk);
int pi_flush_page (struct page *page);


#define PRpage(_x_)	pi_pr_page(__func__, __LINE__, _x_)
#define PRinode(_x_)	pi_pr_inode(__func__, __LINE__, _x_)

void pi_pr_page  (const char *fn, unsigned line, struct page *p);
void pi_pr_inode (const char *fn, unsigned line, struct inode *inode);

#endif
