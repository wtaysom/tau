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

#ifndef _B_INODE_H_
#define _B_INODE_H_

#ifndef _FS_H_
#include <fs.h>
#endif

extern tree_species_s Inode_species;

void    init_inode_store (dev_s *dev);
int     delete_inode (u64 ino);
int     insert_inode (inode_s *inode);
int     update_inode (inode_s *inode);
info_s *find_inode   (u64 ino);
void    dump_inodes  (void);
void	pr_info  (info_s *info);
void	pr_inode (inode_s *inode);

u64 alloc_ino (dev_s *dev);


#endif
