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

#ifndef _MSTRING_H_
#define _MSTRING_H_ 1

#ifndef _MTREE_H_
#include <mtree.h>
#endif

extern tree_s	Tree;

void init_string   (void);
int  delete_string (char *s);
int  find_string   (char *name);
int  insert_string (char *name);
int  next_string (
	u64	key,
	u64	*next_key,
	char	*name);

#endif
