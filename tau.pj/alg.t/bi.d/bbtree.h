/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

/* Binary B Tree */

#ifndef _BBTREE_H_
#define _BBTREE_H_ 1

typedef struct BbNode_s BbNode_s;

typedef struct BbStat_s {
	u64 num_nodes;
	u64 max_depth;
	u64 total_depth;
	u64 num_left;
	u64 num_right;
	u64 deepest;
} BbStat_s;

typedef struct BbTree_s {
	BbNode_s *root;
	u64 num_inserts;
	u64 num_deletes;
	u64 num_rotates;
} BbTree_s;

BbStat_s bb_stats(BbTree_s *tree);

int bb_audit (BbTree_s *tree);
int bb_print (BbTree_s *tree);
int bb_find  (BbTree_s *tree, u64 key);
void bb_insert(BbTree_s *tree, u64 key);
void bb_delete(BbTree_s *tree, u64 key);

void bb_pr_path(BbTree_s *tree, u64 key);

#endif
