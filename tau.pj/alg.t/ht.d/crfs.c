/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <debug.h>
#include <eprintf.h>
#include <style.h>

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

static Buf_s *vol_new(Inode_s *inode);
static Buf_s *ht_new(Inode_s *inode);
static void vol_delete(Buf_s *buf);

Inode_type_s Volume_type = { "Volume",
	.new    = vol_new,
	.read   = dev_fill,
	.flush  = dev_flush,
	.delete = vol_delete};

Inode_type_s Htree_type = { "Htree",
	.new    = ht_new,
	.read   = dev_fill,
	.flush  = dev_flush,
	.delete = vol_delete};

static Volume_s *new_vol (void)
{
	Volume_s	*v;

	v = ezalloc(sizeof(*v));
	v->inode.type   = &Volume_type;
	v->inode.volume = v;
	return v;	
}

Volume_s *crfs_start (char *file)
{
	Volume_s	*v;
	Buf_s		*b;
	Superblock_s	*sb;

	v = new_vol();
	v->dev = dev_open(file);
	b = buf_get( &v->inode, SUPER_BLOCK);
	v->superblock = b;
	sb = b->d;
	assert(sb->magic == CRFS_MAGIC);
	buf_pin(b);
	return v;
}

Volume_s *crfs_create (char *file)
{
	Volume_s	*v;
	Buf_s		*b;
	Superblock_s	*sb;

	v = new_vol();
	v->dev = dev_create(file);
	b = buf_new( &v->inode, SUPER_BLOCK);
	v->superblock = b;
	sb = b->d;
	sb->magic   = CRFS_MAGIC;
	sb->next    = SUPER_BLOCK + 1;
	sb->ht_root = 0;

	buf_pin(b);
	buf_dirty(b);
	dev_flush(b);
	return v;	
}

Htree_s *crfs_htree (Volume_s *v)
{
	Superblock_s	*sb = v->superblock->d;
	Htree_s		*t;

	t = ezalloc(sizeof(*t));	
	t->inode.type = &Htree_type;
	t->inode.volume = v;
	t->ht_root = sb->ht_root;
	return t;
}

static Buf_s *vol_new (Inode_s *inode)
{
	return buf_new(inode, SUPER_BLOCK);
}

static void vol_delete (Buf_s *buf)
{
}

static Buf_s *ht_new (Inode_s *inode)
{
	Volume_s	*v = inode->volume;
	Superblock_s	*sb = v->superblock->d;
	Blknum_t	blknum;
	Buf_s		*b;

	blknum = ++sb->next;
	buf_dirty(v->superblock);
	b = buf_new(inode, blknum);
	buf_dirty(b);
	return b;
}

Blknum_t get_root (Htree_s *t)
{
	return t->ht_root;
}

void set_root (Htree_s *t, Blknum_t blknum)
{
	Volume_s	*v = t->inode.volume;
	Superblock_s	*sb = v->superblock->d;

	sb->ht_root = t->ht_root = blknum;
	dev_flush(v->superblock);
}

	

