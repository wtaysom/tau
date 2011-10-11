/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BTREE_H_
#define _BTREE_H_ 1

typedef struct BtHead_s BtHead_s;

typedef struct BtAudit_s {
	u64 num_nodes;
	u64 max_depth;
	u64 total_depth;
	u64 num_left;
	u64 num_right;
	u64 deepest;
} BtAudit_s;

typedef struct BtStat_s {
	u64 num_inserts;
	u64 num_deletes;
	u64 num_rotates;
} BtStat_s;

typedef struct BtTree_s {
	BtHead_s *root;
	BtStat_s stat;
} BtTree_s;

BtStat_s bt_stats(BtTree_s *tree);
BtAudit_s bt_audit (BtTree_s *tree);

int bt_print (BtTree_s *tree);
int bt_find  (BtTree_s *tree, u64 key);
u64 bt_next  (BtTree_s *tree, u64 key);
int bt_insert(BtTree_s *tree, u64 key);
int bt_delete(BtTree_s *tree, u64 key);

void bt_pr_path(BtTree_s *tree, u64 key);

#endif
