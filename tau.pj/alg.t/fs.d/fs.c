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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>
#include <path.h>

#include <fm.h>
#include <btree.h>
#include <fs.h>
#include <b_inode.h>
#include <b_dir.h>
#include <filemap.h>

typedef struct open_s	open_s;

struct open_s {
	open_s	*o_next;
	u64	o_key;
	s64	o_seek;
	info_s	*o_file;
};

#define OPEN_BUCKETS	37

static open_s	*Open_buckets[OPEN_BUCKETS];
static u64	Next_open_key = 1;

static inline open_s **hash_open (u64 key)
{
	return &Open_buckets[key % OPEN_BUCKETS];
}

static open_s *find_open (u64 key)
{
	open_s	*opn;

	for (opn = *hash_open(key); opn && opn->o_key != key; opn = opn->o_next)
		;
	return opn;
}

static info_s *delete_open (u64 key)
{
	open_s	**bucket = hash_open(key);
	open_s	*next;
	open_s	*prev;
	info_s	*file;

	next = *bucket;
	if (!next) return NULL;

	if (next->o_key == key) {
		*bucket = next->o_next;
	} else {
		for (;;) {
			prev = next;
			next = next->o_next;
			if (!next) return NULL;
			if (next->o_key == key) {
				prev->o_next = next->o_next;
				break;
			}
		}
	}
	next->o_next = NULL;
	file = next->o_file;
	free(next);
	return file;
}

static u64 new_open (info_s *file)
{
	open_s	*opn;
	open_s	**bucket;

	opn = ezalloc(sizeof(*opn));

	opn->o_key = Next_open_key++;
	opn->o_file = file;

	bucket = hash_open(opn->o_key);
	opn->o_next = *bucket;
	*bucket = opn;

	return opn->o_key;
}


static int fs_mknode (info_s *parent, char *name, unint mode)
{
	int	rc;
	info_s	*info;
FN;
	info = new_info(parent, name, mode);
	if (!info) return -ENOMEM;

	rc = insert_dir(parent, info->in_inode.i_no, name);
	if (rc) {
		put_info(info);
binuse();
		return rc;
	}
	rc = insert_inode( &info->in_inode);
	if (rc) {
		put_info(info);
binuse();
		return rc;
	}
binuse();
	return 0;
}

int Close (u64 key)
{
	info_s	*info;
FN;
	info = delete_open(key);
	put_info(info);
	return 0;
}

int Creat (const char *path)
{
	char	name[1024];
	info_s	*parent;
	info_s	*child;
	u64	ino;
FN;
	parent = get_info(ROOT_INO);
	if (!parent) {
		return -ENOENT;
	}
	for (;;) {
		path = parse_path(path, name);
		if (!path) {
			return -EEXIST;
		}
		ino = find_dir(parent, name);
		if (ino) {
			child = get_info(ino);
		} else {
			if (is_last(path)) {
				return fs_mknode(parent, name, S_IFREG);
			}
			return -ENOENT;
		}
		put_info(parent);
		parent = child;
	}
}

int Mknode (const char *path, unint mode)
{
	char	name[1024];
	info_s	*parent;
	info_s	*child;
	u64	ino;
FN;
	parent = get_info(ROOT_INO);
	if (!parent) {
		return -ENOENT;
	}
	for (;;) {
		path = parse_path(path, name);
		if (!path) {
			return -EEXIST;
		}
		ino = find_dir(parent, name);
		if (ino) {
			child = get_info(ino);
		} else {
			if (is_last(path)) {
				return fs_mknode(parent, name, mode);
			}
			return -ENOENT;
		}
		put_info(parent);
		parent = child;
	}
}

int Open (const char *path, u64 *key)
{
	char	name[1024];
	info_s	*parent;
	info_s	*child;
	u64	ino;
FN;
	parent = get_info(ROOT_INO);
	if (!parent) {
		return -ENOENT;
	}
	for (;;) {
		path = parse_path(path, name);
		if (!path) {
			break;
		}
		ino = find_dir(parent, name);
		if (ino) {
			child = get_info(ino);
		} else {
			return -ENOENT;
		}
		put_info(parent);
		parent = child;
	}
	*key = new_open(parent);
	return 0;
}

snint Read (u64 key, void *buf, snint num_bytes)
{
	open_s	*opn;
	info_s	*file;
	u64	seek;
	u64	logical;
	u64	offset;
	unint	chunk;
	snint	bytes_read;
	buf_s	*rbuf;
	u8	*user = buf;
FN;
	opn = find_open(key);
	if (!opn) return -EBADF;

	file = opn->o_file;
	assert(file);

	seek = opn->o_seek;
	if (seek >= file->in_inode.i_eof) return 0;
	if (seek+num_bytes > file->in_inode.i_eof) {
		num_bytes = file->in_inode.i_eof - seek;
	}

	logical = seek >> BLK_SHIFT;
	offset  = seek & BLK_MASK;
	bytes_read = 0;
	for (;;) {
		chunk = BLK_SIZE - offset;
		if (chunk > num_bytes) {
			chunk = num_bytes;
		}
		rbuf = get_block(file, logical);
		assert(rbuf);
		memcpy(user, &((u8 *)rbuf->b_data)[offset], chunk);
		bput(rbuf);

		bytes_read += chunk;
		num_bytes -= chunk;
		if (!num_bytes) break;

		user += chunk;
		++logical;
		offset = 0;
	}
	opn->o_seek = seek + bytes_read;
	return bytes_read;
}

s64 Seek (u64 key, s64 offset)
{
	open_s	*opn;
	info_s	*file;
FN;
	opn = find_open(key);
	if (!opn) return -EBADF;

	file = opn->o_file;
	assert(file);

	opn->o_seek = offset;
	return opn->o_seek;
}

snint Write (u64 key, void *buf, snint num_bytes)
{
	open_s	*opn;
	info_s	*file;
	u64	seek;
	u64	logical;
	u64	offset;
	unint	chunk;
	snint	bytes_written;
	buf_s	*wbuf;
	u8	*user = buf;
FN;
	opn = find_open(key);
	if (!opn) return -EBADF;

	file = opn->o_file;
	assert(file);

	seek = opn->o_seek;

	logical = seek >> BLK_SHIFT;
	offset  = seek & BLK_MASK;
	bytes_written = 0;
	for (;;) {
		chunk = BLK_SIZE - offset;
		if (chunk > num_bytes) {
			chunk = num_bytes;
		}
		wbuf = get_block(file, logical);
		assert(wbuf);
		memcpy( &((u8 *)wbuf->b_data)[offset], user, chunk);
		bdirty(wbuf);
		bput(wbuf);

		bytes_written += chunk;
		num_bytes     -= chunk;
		if (!num_bytes) break;

		user += chunk;
		++logical;
		offset = 0;
	}
	opn->o_seek = seek + bytes_written;
	if (opn->o_seek > file->in_inode.i_eof) {
		file->in_inode.i_eof = opn->o_seek;
		update_inode( &file->in_inode);
	}
	return bytes_written;
}

int Lsdir (const char *path)
{
	char	name[1024];
	info_s	*parent;
	info_s	*child;
	u64	ino;
	u64	key;
	int	rc;
FN;
	parent = get_info(ROOT_INO);
PRp(parent);
	if (!parent) {
		return -ENOENT;
	}
pr_info(parent);
	while (path) {
		path = parse_path(path, name);
		if (!path) {
			break;
		}
		ino = find_dir(parent, name);
		if (ino) {
			child = get_info(ino);
		} else {
			return -ENOENT;
		}
		put_info(parent);
		parent = child;
	}
	for (key = 0; key != ~0ULL;) {
		rc = next_dir(parent, key, &key, name);
		if (rc) break;
		printf("%llx %s\n", key-1, name);
	}
	put_info(parent);
	return 0;
}

int init_fs (const char *dev_name)
{
	dev_s	*dev;
FN;
	dev = bopen(dev_name);
	if (!dev) eprintf("Couldn't open %s:", dev_name);

	init_inode_store(dev);
	return 0;
}
