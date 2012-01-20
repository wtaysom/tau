/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _CRFS_H_
#define _CRFS_H_

#include <style.h>

enum {	BLOCK_SHIFT  = 7,
	BLOCK_SIZE   = 1 << BLOCK_SHIFT};

#define TUESDAY 0
#define WEDNESDAY 0
#define THUSDAY 1

#if TUESDAY
typedef u64 Key_t;
typedef u64 Blknum_t;
#elif WEDNESDAY
typedef u32 Key_t;
typedef u64 Blknum_t;
#elif THURSDAY
typedef u64 Key_t;
typedef u32 Blknum_t;
#else
typedef u32 Key_t;
typedef u32 Blknum_t;
#endif

typedef struct Cache_s	Cache_s;
typedef struct Inode_s	Inode_s;
typedef struct Volume_s	Volume_s;
typedef struct Buf_s	Buf_s;
typedef struct Dev_s	Dev_s;
typedef struct Htree_s	Htree_s;

typedef struct Stat_s {
	struct {
		u64	new;
		u64	deleted;
		u64	split;
		u64	join;
	} leaf, branch;
	u64	insert;
	u64	delete;
	u64	join;
	u64	find;
	u64	next;
} Stat_s;

typedef struct Inode_type_s {
	char	name[8];
	Buf_s	*(*new)(Inode_s *inode);
	void	(*read)(Buf_s *buf);
	void	(*flush)(Buf_s *buf);
	void	(*delete)(Buf_s *buf);
} Inode_type_s;

struct Inode_s {
	Inode_type_s	*type;
	Volume_s	*volume;
};

struct Htree_s {
	Inode_s	inode;
	u64	ht_root;
	Stat_s	stat;
};

struct Volume_s {
	Inode_s	inode;
	Dev_s	*dev;
	Buf_s	*superblock;
	Htree_s	*tree;
	Inode_s	*log;
};

Volume_s *crfs_start(char *file);
Volume_s *crfs_create(char *file);
Htree_s *crfs_htree(Volume_s *v);

Blknum_t get_root(Htree_s *t);
void set_root(Htree_s *t, Blknum_t blknum);

void pr_stats (Htree_s *t);

#endif
