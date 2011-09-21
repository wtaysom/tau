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

typedef struct BiStat_s {
  u64 num_nodes;
  u64 max_depth;
  u64 total_depth;
  u64 num_left;
  u64 num_right;
  Lump_s deepest;
} BiStat_s;

BiStat_s bi_stats(BiNode_s *root);

int bi_audit (BiNode_s *root);
int bi_print (BiNode_s *root);
int bi_find  (BiNode_s *root, Lump_s key, Lump_s *val);
int bi_insert(BiNode_s **root, Rec_s rec);
int bi_delete(BiNode_s **root, Lump_s key);

void bi_pr_path(BiNode_s *root, Lump_s key);

#endif
