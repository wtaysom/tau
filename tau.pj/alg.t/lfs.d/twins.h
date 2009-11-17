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

#ifndef _TWINS_H_
#define _TWINS_H_

#ifndef _BTREE_H_
#include <btree.h>
#endif

#ifndef _LOG_H_
#include <log.h>
#endif

enum { MAX_NAME = 20 /*128*/  };

extern tree_s	TreeA;
extern tree_s	TreeB;

void init_twins   (char *dev_A, char *log_A, char *dev_B, char *log_B);
int  delete_twins (char *s);
int  find_twins   (char *name);
int  insert_twins (char *name);
int  next_twins   (
	u64	key,
	u64	*next_key,
	char	*name);

#endif
