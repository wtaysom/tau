/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <debug.h>

#include "buf.h"
#include "crfs.h"
#include "dev.h"

enum {	SUPER_BLOCK = 1,
	CRFS_MAGIC =  1936093795};

typedef struct Superblock_s {
	u32		magic;
	Blknum_t	next;	/* Next block to be allocated */
	Blknum_t	ht_root;
} Superblock_s;

static Buf_s *vol_new(Crnode_s *crnode);
static Buf_s *ht_new(Crnode_s *crnode);
static void vol_delete(Buf_s *buf);

Crtype_s Volume_type = { "Volume",
	.new    = vol_new,
	.read   = dev_fill,
	.flush  = dev_flush,
	.delete = vol_delete};

Crtype_s Htree_type = { "Htree",
	.new    = ht_new,
	.read   = dev_fill,
	.flush  = dev_flush,
	.delete = vol_delete};

Volume_s Volume = { { &Volume_type } };
Htree_s Htree;

void crfs_start (char *file)
{
	Buf_s		*b;
	Superblock_s	*sb;

	Volume.crnode.type   = &Volume_type;
	Volume.crnode.volume = &Volume;	
	Volume.dev = dev_open(file);
	b = buf_get( &Volume.crnode, SUPER_BLOCK);
	Volume.superblock = b;
	sb = b->d;
	assert(sb->magic == CRFS_MAGIC);
	buf_pin(b);
}

void crfs_create (char *file)
{
	Buf_s		*b;
	Superblock_s	*sb;

	Volume.crnode.type   = &Volume_type;
	Volume.crnode.volume = &Volume;	
	Volume.dev = dev_create(file);
	b = buf_new( &Volume.crnode, SUPER_BLOCK);
	Volume.superblock = b;
	sb = b->d;
	sb->magic   = CRFS_MAGIC;
	sb->next    = SUPER_BLOCK + 1;
	sb->ht_root = 0;

	buf_dirty(b);
	dev_flush(b);	
	buf_pin(b);
}

Htree_s *crfs_htree (void)
{
	Superblock_s	*sb = Volume.superblock->d;
	
	Htree.crnode.type = &Htree_type;
	Htree.crnode.volume = &Volume;
	Htree.ht_root = sb->ht_root;
	return &Htree;
}

static Buf_s *vol_new (Crnode_s *crnode)
{
	return buf_new(crnode, SUPER_BLOCK);
}

static void vol_delete (Buf_s *buf)
{
}

static Buf_s *ht_new (Crnode_s *crnode)
{
	Volume_s	*v = crnode->volume;
	Superblock_s	*sb = v->superblock->d;
	Blknum_t	blknum;
	Buf_s		*b;

	blknum = ++sb->next;
	buf_dirty(v->superblock);
	b = buf_new(crnode, blknum);
	buf_dirty(b);
	return b;
}

Blknum_t get_root (Htree_s *t)
{
	return t->ht_root;
}

void set_root (Htree_s *t, Blknum_t blknum)
{
	Superblock_s	*sb = Volume.superblock->d;

	sb->ht_root = t->ht_root = blknum;
	dev_flush(Volume.superblock);
}

	

