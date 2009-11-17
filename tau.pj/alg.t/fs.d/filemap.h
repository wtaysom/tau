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

#ifndef _FILEMAP_H_
#define _FILEMAP_H_

#ifndef _BLK_H_
#include <blk.h>
#endif

#ifndef _FS_H_
#include <fs.h>
#endif

extern tree_species_s	Filemap_species;

buf_s *get_block (info_s *file, u64 logical);

#endif
