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

#ifndef _CACHE_H_
#define _CACHE_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

#ifndef _Q_H_
#include <q.h>
#endif

#ifndef _ANT_H_
#include <ant.h>
#endif

#ifndef _BTREE_H_
#include <btree.h>
#endif

void   binit    (int num_bufs);
void   blazy    (void);
void   bdump    (void);
void   binuse   (void);
void  *bget     (tree_s *tree, u64 blkno);
void  *bnew     (tree_s *tree, u64 blkno);
void   bsync    (tree_s *tree);
void   bput     (void *data);
void   bdirty   (void *data);
u64    bblkno   (void *data);
void  *bcontext (void *data);
void  *bparent  (void *data);
void  *bdata    (ant_s *ant);
ant_s *bant     (void *data);
void   bchange_blkno     (void *data, u64 blkno);
void   bdepends_on       (void *parent, void *child);
void   bclear_dependency (void *data);

/* Must be implemented by the application using the cache */
void   bread    (tree_s *tree, u64 blkno, void *data);
void   bwrite   (tree_s *tree, u64 blkno, void *data);

#endif
