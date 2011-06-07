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

#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "style.h"
#include "debug.h"
#include "eprintf.h"
#include "bit.h"

#include "pdisk.h"
#include "blk.h"

void	*Dev;
u64	NumBlocks;
u64	NumUsedBlocks;
u64	NumFreeBlocks;

/*
 * Simulates CURRENT_TIME of the kernel
 */
struct timespec pi_time (void)
{
	struct timespec	t;
	struct timeval	v;

	gettimeofday( &v, 0);
	t.tv_sec = v.tv_sec;
	t.tv_nsec = v.tv_usec * 1000;

	return t;
}

void zero_blk (u64 blkno)
{
	buf_s	*buf;

	buf = bnew(Dev, blkno);
	bput(buf);
}

void clear_ends (void)
{
	u64	i;
FN;
	for (i = 0; i < PI_CLEAR_ENDS; ++i) {
		zero_blk(i);
	}
	for (i = NumBlocks - PI_CLEAR_ENDS; i < NumBlocks; ++i) {
		zero_blk(i);
	}
}

void writeSuperBlock (void)
{
	buf_s			*buf;
	pi_super_block_s	*super;
FN;
	buf = bnew(Dev, PI_SUPER_BLK);

	super = buf->b_data;
	PI_SUPER_HEAD(super);
	super->sb_numblocks  = NumBlocks;
	super->sb_nextblock  = PI_START_FREE;
	super->sb_blocksize  = PI_BLK_SIZE;
	bput(buf);
}

void new_inode (
	unsigned	type,
	u64		ino,
	u64		parent,
	char		*name)
{
	buf_s		*buf;
	pi_inode_s	*raw_inode;
	struct timespec	t;

FN;
	buf = bnew(Dev, ino);
	raw_inode = buf->b_data;

	t = pi_time();

	PI_INODE_HEAD(raw_inode);
	raw_inode->t_ino     = ino;
	raw_inode->t_parent  = parent;
	raw_inode->t_mode    = (S_IFDIR | 0755);
	raw_inode->t_link    = 1;
	raw_inode->t_blksize = PI_BLK_SIZE;
	raw_inode->t_size    = 0;
	raw_inode->t_blocks  = 0;
	raw_inode->t_uid     = 0;
	raw_inode->t_gid     = 0;
	raw_inode->t_atime   = t;
	raw_inode->t_mtime   = t;
	raw_inode->t_ctime   = t;
	memset(raw_inode->t_extent, 0, sizeof(raw_inode->t_extent));
	strcpy(raw_inode->t_name, name);
	bput(buf);
}

void writeRootInode (void)
{
FN;
	new_inode (S_IFDIR, PI_ROOT_INO, PI_ROOT_INO, "/");
}

void usage (void)
{
	fprintf(stderr, "usage: %s [-?d] <device> [blocks]\n"
			"\t-?  this message\n"
			"\t-d  debug\n",
			getprogname());
	exit(1);
}

int main (int argc, char *argv[])
{
	int	c;
	u64	size;
	char	*name = ".disk";
	u64	blocks = 0;

	debugoff();
	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "d")) != -1) {
		switch (c) {
		case 'd':
			debugon();
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	if (argv[optind]) {
		name = argv[optind];
		++optind;
	} else {
		usage();
	}
	if (argv[optind]) {
		blocks = atoll(argv[optind]);
	}

	binit(17);
	Dev = bopen(name);
	size = bdevsize(Dev);
	NumBlocks = size ? size : blocks;

	if (NumBlocks < PI_MIN_BLOCKS) {
		fprintf(stderr, "File system only %llu blocks, "
				"must be at least %u blocks.\n",
				NumBlocks, PI_MIN_BLOCKS);
	}
	NumUsedBlocks = ALIGN(NumBlocks, PI_BITS_PER_BLK) / PI_BITS_PER_BLK
			+ PI_START_FREE;
	NumFreeBlocks = NumBlocks - NumUsedBlocks;

	clear_ends();
	writeSuperBlock();
	writeRootInode();

	bclose(Dev);
	return 0;
}
