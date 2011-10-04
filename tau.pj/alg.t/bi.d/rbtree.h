/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _RBTREE_H_
#define _RBTREE_H_ 1

typedef struct RbNode_s RbNode_s;

typedef struct RbStat_s {
	u64 num_nodes;
	u64 max_depth;
	u64 total_depth;
	u64 num_left;
	u64 num_right;
	u64 deepest;
} RbStat_s;

typedef struct RbTree_s {
	addr root;
	u64 num_inserts;
	u64 num_deletes;
	u64 num_rotates;
} RbTree_s;

RbStat_s rb_stats(RbTree_s *tree);

int rb_audit (RbTree_s *tree);
int rb_print (RbTree_s *tree);
int rb_find  (RbTree_s *tree, u64 key);
int rb_insert(RbTree_s *tree, u64 key);
int rb_delete(RbTree_s *tree, u64 key);

void rb_pr_path(RbTree_s *tree, u64 key);

#endif
