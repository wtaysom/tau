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
#include <uuid/uuid.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <bit.h>

#include <tau/disk.h>
#include <blk.h>

void	*Dev;
u64	NumBlocks;
u64	NumUsedBlocks;
u64	NumFreeBlocks;

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
	for (i = 0; i < TAU_CLEAR_ENDS; ++i) {
		zero_blk(i);
	}
	for (i = NumBlocks - TAU_CLEAR_ENDS; i < NumBlocks; ++i) {
		zero_blk(i);
	}
}

void write_super_block (guid_t vol_guid)
{
	buf_s		*buf;
	tau_bag_s	*tbag;
FN;
	buf = bnew(Dev, TAU_SUPER_BLK);

	tbag = buf->b_data;
	TAU_SUPER_HEAD(tbag, TAU_SUPER_BLK);
	uuid_copy(tbag->tb_guid_vol, vol_guid);
	uuid_generate(tbag->tb_guid_bag);

	tbag->tb_state        = TAU_BAG_NEW;
	tbag->tb_num_bags     = 0;
	tbag->tb_num_shards   = 0;
	tbag->tb_num_replicas = 0;

	tbag->tb_numblocks    = NumBlocks;
	tbag->tb_nextblock    = TAU_START_FREE;
	tbag->tb_log_start    = TAU_START_LOG;
	tbag->tb_log_size     = TAU_STAGE_SIZE * TAU_STAGES;
	bput(buf);
}

void usage (void)
{
	fprintf(stderr, "usage: %s [-?d] [-u <uuid>] <device> [blocks]\n"
			"\t-?  this message\n"
			"\t-d  debug\n"
			"\t-u  universally unique id for volume\n",
			getprogname());
	exit(1);
}

int main (int argc, char *argv[])
{
	int	c;
	u64	size;
	guid_t	vol_guid;
	char	guid_name[2*36];
	char	*name = ".disk";
	u64	blocks = 0;
	int	rc;

	debugoff();
	setprogname(argv[0]);
	uuid_generate(vol_guid);

	while ((c = getopt(argc, argv, "du:")) != -1) {
		switch (c) {
		case 'd':
			debugon();
			break;
		case 'u':
			rc = uuid_parse(optarg, vol_guid);
			if (rc) {
				printf("couldn't parse %s\n", optarg);
				exit(2);
			}
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

	if (NumBlocks < TAU_MIN_BLOCKS) {
		fprintf(stderr, "File system only %llu blocks, "
				"must be at least %u blocks.\n",
				NumBlocks, TAU_MIN_BLOCKS);
	}
	NumUsedBlocks = ALIGN(NumBlocks, TAU_BITS_PER_BLK) / TAU_BITS_PER_BLK
			+ TAU_START_FREE;
	NumFreeBlocks = NumBlocks - NumUsedBlocks;

	clear_ends();
	write_super_block(vol_guid);

	bclose(Dev);

	uuid_unparse(vol_guid, guid_name);
	printf("Using %s for volume guid\n", guid_name);
	return 0;
}
