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

#ifndef _DIR_H_
#define _DIR_H_ 1

#ifndef _MTREE_H_
#include <mtree.h>
#endif

typedef struct dir_s {
	u32	d_ino;
	u8	d_type;
	char	d_name[256];
} dir_s;

extern tree_s	Tree;

void init_dir   (void);
int  delete_dir (char *s);
int  find_dir   (char *name);
int  insert_dir (char *name, u32 ino, u8 type);
int  next_dir (
	u32	key,
	u32	*next_key,
	dir_s	*dir);

#endif
