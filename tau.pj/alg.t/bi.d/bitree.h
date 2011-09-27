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

typedef struct BiStat_s {
  u64 num_nodes;
  u64 max_depth;
  u64 total_depth;
  u64 num_left;
  u64 num_right;
  Lump_s deepest;
} BiStat_s;

typedef struct BiTree_s {
  BiNode_s *root;
  u64 num_inserts;
  u64 num_deletes;
  u64 num_rotates;
} BiTree_s;

BiStat_s bi_stats(BiTree_s *tree);

int bi_audit (BiTree_s *tree);
int bi_print (BiTree_s *tree);
int bi_find  (BiTree_s *tree, Lump_s key, Lump_s *val);
int bi_insert(BiTree_s *tree, Rec_s rec);
int bi_delete(BiTree_s *tree, Lump_s key);

void bi_pr_path(BiTree_s *tree, Lump_s key);

#endif
