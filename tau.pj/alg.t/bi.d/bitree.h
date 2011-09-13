/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BITREE_H_
#define _BITREE_H_ 1

#include <lump.h>

typedef struct Rec_s {
  Lump_s key;
  Lump_s val;
} Rec_s;

typedef struct BiNode_s BiNode_s;

struct BiNode_s {
  BiNode_s *left;
  BiNode_s *right;
  Rec_s rec;
};

typedef struct BiTree_s {
  BiNode_s *root;
} BiTree_s;

int bi_audit  (BiTree_s t);
int bi_print  (BiTree_s t);
int bi_stats  (BiTree_s t);
int bi_find   (BiTree_s t, Lump_s key, Lump_s *val);
int bi_insert (BiTree_s *t, Rec_s rec);
int bi_delete (BiTree_s *t, Lump_s key);

#endif
