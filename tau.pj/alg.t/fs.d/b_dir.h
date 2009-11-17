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

#ifndef _B_DIR_H_
#define _B_DIR_H_

#ifndef _FS_H_
#include <fs.h>
#endif

extern tree_species_s	Dir_species;

int delete_dir (info_s *parent, char *name);
u64 find_dir   (info_s *parent, char *name);
int insert_dir (info_s *parent, u64 ino, char *name);
int next_dir (
	info_s	*parent,
	u64	key,
	u64	*next_key,
	char	*name);

void dump_dir (tree_s *tree, u64 root);

#endif
