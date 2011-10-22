/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BTREE_H_
#define _BTREE_H_ 1

typedef struct BtNode_s BtNode_s;

typedef struct BtAudit_s {
	u64 depth;
	u64 num_lf_keys;
	u64 num_br_keys;
	u64 num_branches;
	u64 num_leaves;
	u64 max_key;
} BtAudit_s;

typedef struct BtStat_s {
	u64 num_inserts;
	u64 num_deletes;
	u64 num_grow;
	u64 num_shrink;
	u64 num_split;
	u64 num_joins;
} BtStat_s;

typedef struct BtTree_s {
	BtNode_s *root;
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
