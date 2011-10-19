/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "bttree.h"

enum {	NUM_KEYS = 7,
	KEYS_LOWER_HALF = NUM_KEYS / 2,
	KEYS_UPPER_HALF = NUM_KEYS - KEYS_LOWER_HALF,
	NUM_TWIGS = NUM_KEYS * sizeof(u64) / (sizeof(void *) + sizeof(u64)),
	TWIGS_LOWER_HALF = NUM_TWIGS / 2,
	TWIGS_UPPER_HALF = NUM_TWIGS - TWIGS_LOWER_HALF };


#if 0
typedef struct Twig_s {
	union {
		u64 key;
		struct {
			bool is_leaf;
			u16 num;
		};
	};
	BtNode_s *node;
} Twig_s;
#endif

typedef struct Twig_s {
	u64 key;
	BtNode_s *node;
} Twig_s;

struct BtNode_s {
	bool is_leaf;
	u16 num;
	union {
		u64 key[NUM_KEYS];
		struct {
			BtNode_s *first;
			Twig_s twig[NUM_TWIGS];
		};
	};
};

CHECK_CONST(sizeof(BtNode_s) == (NUM_KEYS + 1) * sizeof(u64));
	

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

static void pr_leaf (BtNode_s *leaf, int indent)
{
	int i;

	for (i = 0; i < leaf->num; i++) {
		pr_indent(indent);
		printf("%llu\n", leaf->key[i]);
	}
}

static void pr_node(BtNode_s *node, int indent);

static void pr_branch (BtNode_s *branch, int indent)
{
	int i;

	pr_node(branch->first, indent + 1);
	for (i = 0; i < branch->num; i++) {
		pr_indent(indent);
		printf("%llu\n", branch->twig[i].key);
		pr_node(branch->twig[i].node, indent + 1);
	}
}

static void pr_node (BtNode_s *node, int indent)
{
	if (!node) return;
	pr_indent(indent); printf("%p\n", node);
	if (node->is_leaf) {
		pr_leaf(node, indent + 1);
	} else {
		pr_branch(node, indent + 1);
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

static BtNode_s *new_leaf (void)
{
	BtNode_s *node = ezalloc(sizeof(*node));
	node->is_leaf = TRUE;
	return node;
}

static BtNode_s *new_branch (void)
{
	BtNode_s *node = ezalloc(sizeof(*node));
	node->is_leaf = FALSE;
	return node;
}

static void leaf_insert (BtNode_s *leaf, u64 key)
{
	int i;

	assert(leaf->num < NUM_KEYS);
	for (i = 0; i < leaf->num; i++) {
		if (key < leaf->key[i]) {
			memmove(&leaf->key[i+1], &leaf->key[i],
				sizeof(u64) * (leaf->num - i));
			++leaf->num;
			leaf->key[i] = key;
			return;
		}
	}
	leaf->key[leaf->num++] = key;
}

static void branch_insert (BtNode_s *parent, u64 key, BtNode_s *node)
{
	int i;

	assert(parent->num < NUM_KEYS);
	for (i = 0; i < parent->num; i++) {
		if (key < parent->key[i]) {
			memmove(&parent->key[i+1], &parent->key[i],
				sizeof(Twig_s) * (parent->num - i));
			parent->twig[i].key = key;
			parent->twig[i].node = node;
			++parent->num;
			return;
		}
	}
	parent->twig[parent->num].key = key;
	parent->twig[parent->num].node = node;
	++parent->num;
}

static bool is_full (BtNode_s *node)
{
	return node->num == (node->is_leaf ? NUM_KEYS : NUM_TWIGS);
}

static BtNode_s *grow (BtNode_s *node)
{
	BtNode_s *branch = new_branch();
	branch->first = node;
	return branch;
}

static BtNode_s *lookup (BtNode_s *branch, u64 key)
{
	BtNode_s *node = branch->first;
	Twig_s	*end = &branch->twig[branch->num];
	Twig_s	*twig;

	for (twig = branch->twig; twig < end; twig++) {
		if (key < twig->key) {
			return node;
		}
		node = twig->node;
	}
	return node;
}

static BtNode_s *split_leaf (BtNode_s *parent, BtNode_s *node)
{
	BtNode_s *sibling = new_leaf();

PRp(sibling);
	memmove(sibling->key, &node->key[KEYS_LOWER_HALF],
		sizeof(u64) * KEYS_UPPER_HALF);
	node->num = KEYS_LOWER_HALF;
	sibling->num = KEYS_UPPER_HALF;
	branch_insert(parent, sibling->key[0], sibling);
	return parent;	
}

static BtNode_s *split_branch (BtNode_s *parent, BtNode_s *node)
{
	BtNode_s *sibling = new_branch();

	memmove(sibling->twig, &node->twig[TWIGS_LOWER_HALF],
		sizeof(Twig_s) * TWIGS_UPPER_HALF);
	node->num = TWIGS_LOWER_HALF;
	sibling->num = TWIGS_UPPER_HALF;
	branch_insert(parent, sibling->twig[0].key, node);
	return parent;	
}

static BtNode_s *split (BtNode_s *parent, BtNode_s *node)
{
	if (node->is_leaf) {
		return split_leaf(parent, node);
	} else {
		return split_branch(parent, node);
	}
}

int bt_insert (BtTree_s *tree, u64 key)
{
	BtNode_s *node;
	BtNode_s *parent;

	node = tree->root;
	if (!node) {
		tree->root = new_leaf();
		leaf_insert(tree->root, key);
		return 0;
	}
	if (is_full(node)) {
		node = tree->root = grow(node);
	}
	for (;;) {
		if (node->is_leaf) {
			leaf_insert(node, key);
			return 0;
		}
		parent = node;
		node = lookup(node, key);
		if (is_full(node)) {
			node = split(parent, node);
		}
	}
}

#if 0
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
#else 
int bt_delete (BtTree_s *tree, u64 key) { fatal("Not implemeted"); return 0; }
u64 bt_next (BtTree_s *tree, u64 key) { fatal("Not implemeted"); return 0; }
BtAudit_s bt_audit (BtTree_s *tree)
{
	BtAudit_s audit = { 0 };
	fatal("Not implemented");
	return audit;
}
#endif
