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
#include <filemap.h>

typedef struct fmap_s {
	u64	fm_logical;
	u64	fm_physical;
	u64	fm_length;
} fmap_s;

//===================================================================

static int pack_fmap (void *to, void *from, unint len)
{
FN;
	memcpy(to, from, len);
	return 0;
}

int insert_fmap (info_s *file, u64 logical, u64 physical)
{
	int	rc;
	fmap_s	fmap;
FN;
	fmap.fm_logical = logical;
	fmap.fm_physical = physical;
	fmap.fm_length = 1;
	rc = insert( &file->in_tree, logical, &fmap, sizeof(fmap));
	return rc;
}

//===================================================================

typedef struct find_fmap_s {
	info_s	*file;
	buf_s	*buf;
	u64	logical;
} find_fmap_s;

static int find_fmap (
	void	*data,
	u64	rec_key,
	void	*rec,
	unint	len)
{
	find_fmap_s	*f = data;
	fmap_s		*fmap = rec;
FN;
	if (f->logical != rec_key) {
		return qERR_NOT_FOUND;
	}
	/*
	 * Found the file map, get a buffer
	 * XXX probably not right. Want to get buffer when no other
	 * resources are being held.
	 */
	f->buf = bget(f->file->in_tree.t_dev, fmap->fm_physical);

	return 0;
}

buf_s *get_block (info_s *file, u64 logical)
{
	find_fmap_s	f;
	int		rc;

	f.file = file;
	f.logical = logical;
	f.buf = NULL;
	rc = search( &file->in_tree, logical, find_fmap, &f);
	if (!rc) return f.buf;

	f.buf = alloc_block(file->in_tree.t_dev);
	if (!f.buf) return NULL;

	rc = insert_fmap(file, logical, f.buf->b_blkno);
	assert(rc == 0);

	return f.buf;
}

//===================================================================

void dump_fmap (tree_s *tree, u64 rec_key, void *rec, unint len)
{
	fmap_s	*fmap = rec;

	printf("%lld %lld %lld\n",
		fmap->fm_logical,
		fmap->fm_physical,
		fmap->fm_length);
}

void dump_filemap (info_s *file)
{
	dump_tree( &file->in_tree);
}

//===================================================================

static u64 root_fmap (tree_s *tree)
{
	info_s	*file = container(tree, info_s, in_tree);

	return file->in_inode.i_root;
}

static int change_root_fmap (tree_s *tree, buf_s *root)
{
	info_s	*file = container(tree, info_s, in_tree);
	int	rc;
FN;
	file->in_inode.i_root = root->b_blkno;
	rc = update_inode( &file->in_inode);
	return rc;
}

//===================================================================

tree_species_s Filemap_species = {
	"Filemap",
	ts_dump:	dump_fmap,
	ts_root:	root_fmap,
	ts_change_root:	change_root_fmap,
	ts_pack:	pack_fmap};
