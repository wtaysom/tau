/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include "buf.h"
#include "crfs.h"
#include "dev.h"

enum { SUPER_BLOCK = 1 };

typedef struct Superblock_s {
	u64		magic;
	Blknum_t	next;	/* Next block to be allocated */
} Superblock_s;

static Buf_s *vol_new(Crnode_s *crnode);
static int vol_read(Buf_s *buf);
static int vol_flush(Buf_s *buf);
static int vol_delete(Buf_s *buf);

Crtype_s Volume_type = { "Volume",
	.new    = vol_new,
	.read   = vol_read,
	.flush  = vol_flush,
	.delete = vol_delete};
	

Volume_s Volume = { { &Volume_type } };
Htree_s Htree;

void crfs_start (char *file)
{
	Volume.dev = dev_open(file);
}

void crfs_create (char *file)
{
	Volume.dev = dev_create(file);
	
}

Htree_s *crfs_htree (void)
{
	return &Htree;
}

static Buf_s *vol_new (Crnode_s *crnode)
{
}

static int vol_read (Buf_s *buf)
{
}

static int vol_flush (Buf_s *buf)
{
}

static int vol_delete (Buf_s *buf)
{
}

