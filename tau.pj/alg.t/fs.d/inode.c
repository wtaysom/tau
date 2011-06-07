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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <style.h>
#include <debug.h>
#include <eprintf.h>
#include <btree.h>
#include <fs.h>
#include <b_inode.h>
#include <b_dir.h>
#include <filemap.h>

tree_s	Inode_tree;

static void add_info (u64 ino, info_s *info);


//===================================================================

static tree_species_s *get_species (unint mode)
{
	if (S_ISREG(mode)) return &Filemap_species;
	if (S_ISDIR(mode)) return &Dir_species;
	return NULL;
}

static void init_inode (
	inode_s	*inode,
	u64	ino,
	unint	mode,
	char	*name)
{
	inode->i_no = ino;
	inode->i_mode = mode;
	inode->i_root = 0;
	strcpy(inode->i_name, name);
}

info_s *alloc_info (unint name_len, tree_species_s *species, dev_s *dev)
{
	info_s	*info;
FN;
	info = ezalloc(sizeof(info_s) + name_len);

	init_tree( &info->in_tree, species, dev);
	return info;
}

info_s *new_info (info_s *parent, char *name, unint mode)
{
	info_s	*info;
	u64	ino;
FN;
	ino = alloc_ino(parent->in_tree.t_dev);

	info = alloc_info(strlen(name) + 1, get_species(mode),
				parent->in_tree.t_dev);

	init_inode( &info->in_inode, ino, mode, name);

	add_info(ino, info);

	return info;
}

//===================================================================

typedef struct delete_search_s {
	u64	key;
} delete_search_s;

static int delete_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	delete_search_s	*s = data;
FN;
	if (s->key != rec_key) {
		return qERR_NOT_FOUND;
	}
	return 0;
}

int delete_inode (u64 ino)
{
	int		rc;
	delete_search_s	s;
FN;
	s.key = ino;
	rc = search( &Inode_tree, s.key, delete_search, &s);
	if (rc) {
		printf("delete_inode [%d] couldn't find %llx\n", rc, ino);
		return rc;
	}
	rc = delete( &Inode_tree, s.key);
	if (rc) {
		printf("Failed to delete inode %llx err=%d\n",
			ino, rc);
	}
	return 0;
}

//===================================================================

static int pack_inode (void *to, void *from, unint len)
{
FN;
	memcpy(to, from, len);
	return 0;
}

int insert_inode (inode_s *inode)
{
	int	rc;
	unint	len;
FN;
	len = sizeof(*inode) + strlen(inode->i_name) + 1;
	rc = insert( &Inode_tree, inode->i_no, inode, len);
	return rc;
}

//===================================================================

int update_inode (inode_s *inode)
{
	int	rc;
FN;
	rc = delete_inode(inode->i_no);
	if (rc) return rc;

	rc = insert_inode(inode);
	return rc;
}

//===================================================================

typedef struct find_search_s {
	info_s	*info;
	u64	ino;
} find_search_s;

static int find_search (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	find_search_s	*s = data;
	inode_s		*inode = rec;
	info_s		*info;
FN;
	if (s->ino != rec_key) {
		return qERR_NOT_FOUND;
	}
	/*
	 * Found the inode, allocate a structure and fill it in
	 */
	info = alloc_info(len - sizeof(inode_s), get_species(inode->i_mode),
				Inode_tree.t_dev);
	memcpy( &info->in_inode, inode, len);

	s->info = info;
	return 0;
}

info_s *find_inode (u64 ino)
{
	find_search_s	s;
	int		rc;

	s.ino = ino;
	rc = search( &Inode_tree, ino, find_search, &s);
	return rc ? NULL : s.info;
}

//===================================================================

enum { NUM_BUCKETS = 3 };

static info_s	*Bucket[NUM_BUCKETS];

static info_s **hash_ino (u64 ino)
{
	return &Bucket[ino % NUM_BUCKETS];
}

static info_s *find_info (u64 ino)
{
	info_s	**bucket;
	info_s	*info;
	info_s	*prev;
FN;
	bucket = hash_ino(ino);
	info = *bucket;
	if (!info) return NULL;
	if (info->in_inode.i_no == ino) {
		++info->in_use;
		return info;
	}
	for (;;) {
		prev = info;
		info = prev->in_next;
		if (!info) return NULL;
		if (info->in_inode.i_no == ino) {
			prev->in_next = info->in_next;
			info->in_next = *bucket;
			*bucket = info;
			++info->in_use;
			return info;
		}
	}
}

static void rmv_info (info_s *info)
{
	info_s	**bucket;
	info_s	*prev;
FN;
	bucket = hash_ino(info->in_inode.i_no);
	prev = *bucket;
	if (!prev) return;
	if (prev == info) {
		*bucket = info->in_next;
		return;
	}
	for (;;) {
		info = prev->in_next;
		if (!info) return;
		if (info == prev) {
			prev->in_next = info->in_next;
			return;
		}
		prev = info;
	}
}

static void add_info (u64 ino, info_s *info)
{
	info_s	**bucket;
FN;
	bucket = hash_ino(ino);
	info->in_next = *bucket;
	*bucket = info;
	++info->in_use;
}

info_s *get_info (u64 ino)
{
	info_s	*info;
FN;
PRd(ino);
	info = find_info(ino);
	if (info) return info;

	info = find_inode(ino);
	if (!info) return NULL;

	add_info(ino, info);
	return info;
}

void put_info (info_s *info)
{
FN;
	assert(info->in_use);
	if (!info) return;
	if (--info->in_use) return;
	rmv_info(info);
}

//===================================================================

void pr_inode (inode_s *inode)
{
	printf("%lld %lld %s\n", inode->i_no, inode->i_root, inode->i_name);
}

void pr_info (info_s *info)
{
	pr_inode( &info->in_inode);
}

void dump_inode (tree_s *tree, u64 rec_key, void *rec, unint len)
{
	inode_s	*inode = rec;

	printf("%lld %c %lld %s\n",
		inode->i_no,
		S_ISDIR(inode->i_mode) ? 'd' : 'f',
		inode->i_root, inode->i_name);
	if (S_ISDIR(inode->i_mode)) {
		dump_dir(tree, inode->i_root);
	}
}

void dump_inodes (void)
{
	dump_tree( &Inode_tree);
}

//===================================================================

enum {	SUPER_BLOCK = 1,
	SUPER_MAGIC = 0x1111111111111111ULL };

typedef struct super_s {
	u64	sp_magic;
	u64	sp_root;
	u64	sp_next;
	u64	sp_ino;
} super_s;

static u64 root_inode (tree_s *tree)
{
	super_s	*super;
	buf_s	*buf;
	u64	root;
FN;
	buf = bget(tree->t_dev, SUPER_BLOCK);
	if (!buf) {
		eprintf("grow_string: no super block");
		return qERR_NOT_FOUND;
	}
	super = buf->b_data;
	root = super->sp_root;
	bput(buf);
	return root;
}

static int change_root_inode (tree_s *tree, buf_s *root)
{
	super_s	*super;
	buf_s	*buf;
FN;
	buf = bget(tree->t_dev, SUPER_BLOCK);
	if (!buf) {
		eprintf("grow_string: no super block");
		return qERR_NOT_FOUND;
	}
	super = buf->b_data;
	super->sp_root = root->b_blkno;
	bdirty(buf);
	bput(buf);
	return 0;
}

u64 alloc_ino (dev_s *dev)
{
	super_s	*super;
	buf_s	*buf;
	u64	ino;
FN;
	buf = bget(dev, SUPER_BLOCK);
	super = buf->b_data;

	if (super->sp_magic != SUPER_MAGIC) {
		eprintf("no super block");
	}
	ino = super->sp_ino++;
	bdirty(buf);
	bput(buf);

	return ino;
}

buf_s *alloc_block (dev_s *dev)
{
	super_s	*super;
	buf_s	*buf;
	u64	blkno;
FN;
	buf = bget(dev, SUPER_BLOCK);
	super = buf->b_data;

	if (super->sp_magic != SUPER_MAGIC) {
		eprintf("no super block");
	}
	blkno = super->sp_next++;
PRd(blkno);
	bdirty(buf);
	bput(buf);

	buf = bnew(dev, blkno);
	bdirty(buf);
	return buf;
}

static void init_root_inode (void)
{
	struct {
		inode_s	inode;
		char	name[1];
	} root;
FN;
	init_inode( &root.inode, ROOT_INO, S_IFDIR, "");
	root.name[0] = '\0';
	insert_inode( &root.inode);
}

static void init_super_block (void)
{
	super_s	*super;
	buf_s	*buf;
FN;
	buf = bget(Inode_tree.t_dev, SUPER_BLOCK);
	super = buf->b_data;
	if (super->sp_magic != SUPER_MAGIC) {
		super->sp_magic = SUPER_MAGIC;
		super->sp_root  = 0;
		super->sp_next  = SUPER_BLOCK + 1;
		super->sp_ino   = ROOT_INO + 1;

		bdirty(buf);
	}
	bput(buf);

	init_root_inode();
}

void init_inode_store (dev_s *dev)
{
FN;
	init_tree( &Inode_tree, &Inode_species, dev);

	init_super_block();
}


//===================================================================

tree_species_s Inode_species = {
	"Inode",
	ts_dump:	dump_inode,
	ts_root:	root_inode,
	ts_change_root:	change_root_inode,
	ts_pack:	pack_inode};

