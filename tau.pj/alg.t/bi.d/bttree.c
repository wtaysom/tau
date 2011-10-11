/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "bttree.h"

enum {	NUM_KEYS = 15,
	NUM_TWIGS = 7 };

struct BtHead_s {
	bool is_leaf;
	u16 num;
};

typedef struct Leaf_s {
	BtHead_s head;
	u64 key[NUM_KEYS];
} Leaf_s;

typedef struct Twig_s {
	BtHead_s *hp;
	u64 key;
} Twig_s;

typedef struct Branch_s {
	BtHead_s head;
	BtHead_s *first;
	Twig_s twig[NUM_TWIGS];
} Branch_s;
	

BtStat_s bt_stat (BtTree_s *tree)
{
	return tree->stat;
}

static void pr_indent (int indent)
{
	while (indent-- > 0) {
		printf("    ");
	}
}

static void pr_key (u64 key, int indent)
{
	pr_indent(indent);
	printf("%llu\n", key);
}

static void pr_leaf (Leaf_s *leaf, int indent)
{
	int i;

	for (i = 0; i < leaf->head.num; i++) {
		pr_indent(indent);
		printf("%llu\n", leaf->key[i]);
	}
}

static void pr_node(BtHead_s *head, int indent);

static void pr_branch (Branch_s *branch, int indent)
{
	int i;

	pr_node(branch->first, indent + 1);
	for (i = 0; i < branch->head.num; i++) {
		pr_indent(indent);
		printf("%llu\n", branch->twig[i].key);
		pr_node(branch->twig[i].hp, indent + 1);
	}
}

static void pr_node (BtHead_s *head, int indent)
{
	if (!head) return;
	if (head->is_leaf) {
		pr_leaf((Leaf_s *)head, indent);
	} else {
		pr_branch((Branch_s *)head, indent);
	}
}

int bt_print (BtTree_s *tree)
{
	pr_node(tree->root, 0);
	return 0;
}

#if 0
void bt_pr_path (BtTree_s *tree, u64 key)
{
	BtNode_s *node = tree->root;
	while (node) {
		printf("%llu", node->key);
		if (key == node->key) {
			printf("\n");
			return;
		}
		printf(" ");
		if (key < node->key) {
			node = node->left;
		} else {
			node = node->right;
		}
	}
}

static void node_audit (BtNode_s *node, BtAudit_s *audit, int depth)
{
	if (!node) return;
	++audit->num_nodes;
	if (depth > audit->max_depth) {
		audit->max_depth = depth;
		audit->deepest = node->key;
	}
	audit->total_depth += depth;
	if (node->left) {
		++audit->num_left;
		node_audit(node->left, audit, depth + 1);
	}
	if (node->right) {
		++audit->num_right;
		node_audit(node->right, audit, depth + 1);
	}
}
	
BtAudit_s bt_audit (BtTree_s *tree)
{
	BtAudit_s audit = { 0 };
	node_audit(tree->root, &audit, 1);
	return audit;
}

int bt_find (BtTree_s *tree, u64 key)
{
	BtNode_s *node = tree->root;
	while (node) {
		if (key == node->key) {
			return 0;
		}
		if (key < node->key) {
			node = node->left;
		} else {
			node = node->right;
		}
	}
	return -1;
}

u64 bt_first (BtTree_s *tree)
{
	BtNode_s *node = tree->root;
	if (!node) return 0;
	while (node->left) node = node->left;
	return node->key;
}

u64 bt_next (BtTree_s *tree, u64 key)
{
	BtNode_s *node = tree->root;
	BtNode_s *nextrt = NULL;

	if (key == 0) return bt_first(tree);
	while (node) {
		if (key < node->key) {
			nextrt = node;
			node = node->left;
		} else if (key == node->key) {
			if (node->right) {
				node = node->right;
				while (node->left) node = node->left;
				return node->key;
			} else {
				if (nextrt) {
					return nextrt->key;
				} else {
					return 0;
				}
			}
		} else {
			node = node->right;
		}
	}
	return 0;
}
#endif

static Leaf_s *leaf_new (void)
{
	Leaf_s *leaf = ezalloc(sizeof(*leaf));
	leaf->is_leaf = TRUE;
	return leaf;
}

static Branch_s *branch_new (void)
{
	Branch_s *branch = ezalloc(sizeof(*branch));
	branch->is_leaf = FALSE;
	return branch;
}

static void leaf_insert (Leaf_s *leaf, u64 key)
{
	assert(leaf->head.num < NUM_KEYS);
	for (i = 0; i < leaf->head.num; i++) {
		if (key < leaf->key[i]) {
			memmove(&leaf->key[i+1], &leaf->key[i], leaf->head.num - i);
			++leaf->head.num;
			leaf->key[i] = key;
			return;
		}
	}
	leaf->key[leaf->head.num++] = key;
}

static bool is_full (BtHead_s *head)
{
	return head->num == (head->is_leaf ? NUM_KEYS : NUM_TWIGS);
}

static BtHead_s *grow (BtHead_s *head)
{
	Branch_s *branch = branch_new();
	branch->first = head;
	return branch;
}

static BtHead_s *lookup (Head_s *head, u64 key)
{
}

int bt_insert (BtTree_s *tree, u64 key)
{
	BtHead_s *head;

	if (!tree->root) {
		tree->root = leaf = leaf_new;
		leaf_insert(leaf, key);
		return 0;
	}
	head = tree->root;
	if (is_full(head)) {
		head = tree->root = grow(head);
	}
	for (;;) {
		if (head->is_leaf) {
			leaf_insert((Leaf_s *)head, key);
			return 0;
		}
		parent = head;
		head = lookup((Branch_s *)head, key);
		if (is_full(head)) {
			split
	}
	*np = bt_new(key);
	return 0;
}

static void delete_node (BtNode_s **dp)
{
	BtNode_s **np = dp;
	BtNode_s *deletee;
	BtNode_s *node;
	deletee = *dp;
	if (!deletee->right) {
		*np = deletee->left;
		free(deletee);
		return;
	}
	np = &deletee->right;
	node = *np;
	if (!node->left) {
		node->left = deletee->left;
		*dp = node;
		free(deletee);
		return;
	}
	for (;;) {
		np = &node->left;
		node = *np;
		if (!node->left) {
			*dp = node;
			*np = node->right;
			node->right = deletee->right;
			node->left = deletee->left;
			free(deletee);
			return;
		}
	}
}

int bt_delete (BtTree_s *tree, u64 key)
{
	BtNode_s **np = &tree->root;
	for (;;) {
		BtNode_s *node = *np;
		if (!node) fatal("Key %llu not found", key);
		if (key == node->key) break;
		if (key < node->key) {
			np = &node->left;
		} else {
			np = &node->right;
		}
	}
	delete_node(np);
	return 0;
}
