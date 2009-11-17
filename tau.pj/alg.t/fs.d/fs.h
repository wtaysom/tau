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

#ifndef _FS_H_
#define _FS_H_ 1

#ifndef _BTREE_H_
#include <btree.h>
#endif

enum {	ROOT_INO = 1 };

typedef struct inode_s {
	u64	i_no;
	u64	i_mode;
	u64	i_eof;
	u64	i_root;
	char	i_name[0];
} inode_s;

typedef struct info_s	info_s;
struct info_s {
	info_s	*in_next;
	tree_s	in_tree;	// Why isn't this a pointer?
	u64	in_use;
	inode_s	in_inode;	// Must be last
};

info_s *get_info (u64 ino);
void    put_info (info_s *info);
info_s *new_info (info_s *parent, char *name, unint mode);


#endif
