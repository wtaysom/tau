/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _IBITREE_H_
#define _IBITREE_H_ 1

typedef struct iBiNode_s iBiNode_s;

typedef struct iBiStat_s {
  u64 num_nodes;
  u64 max_depth;
  u64 total_depth;
  u64 num_left;
  u64 num_right;
  u64 deepest;
} iBiStat_s;

typedef struct iBiTree_s {
  iBiNode_s *root;
  u64 num_inserts;
  u64 num_deletes;
  u64 num_rotates;
} iBiTree_s;

iBiStat_s ibi_stats(iBiTree_s *tree);

int ibi_audit (iBiTree_s *tree);
int ibi_print (iBiTree_s *tree);
int ibi_find  (iBiTree_s *tree, u64 key);
int ibi_insert(iBiTree_s *tree, u64 key);
int ibi_delete(iBiTree_s *tree, u64 key);

void ibi_pr_path(iBiTree_s *tree, u64 key);

#endif
