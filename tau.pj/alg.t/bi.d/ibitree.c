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

#include "ibitree.h"

struct iBiNode_s {
	iBiNode_s *left;
	iBiNode_s *right;
	u64 key;
};

int ibi_audit (iBiTree_s *tree)
{
	return 0;
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

static void pr_node (iBiNode_s *node, int indent)
{
	if (!node) return;
	pr_node(node->left, indent + 1);
	pr_key(node->key, indent);
	pr_node(node->right, indent + 1);
}

int ibi_print (iBiTree_s *tree)
{
	pr_node(tree->root, 0);
	return 0;
}

void ibi_pr_path (iBiTree_s *tree, u64 key)
{
	iBiNode_s *node = tree->root;
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

static void node_stat (iBiNode_s *node, iBiStat_s *s, int depth)
{
	if (!node) return;
	++s->num_nodes;
	if (depth > s->max_depth) {
		s->max_depth = depth;
		s->deepest = node->key;
	}
	s->total_depth += depth;
	if (node->left) {
		++s->num_left;
		node_stat(node->left, s, depth + 1);
	}
	if (node->right) {
		++s->num_right;
		node_stat(node->right, s, depth + 1);
	}
}
	
iBiStat_s ibi_stats (iBiTree_s *tree)
{
	iBiStat_s stat = { 0 };
	node_stat(tree->root, &stat, 1);
	return stat;
}

int ibi_find (iBiTree_s *tree, u64 key)
{
	iBiNode_s *node = tree->root;
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

u64 ibi_first (iBiTree_s *tree)
{
	iBiNode_s *node = tree->root;
	if (!node) return 0;
	while (node->left) node = node->left;
	return node->key;
}

u64 ibi_next (iBiTree_s *tree, u64 key)
{
	iBiNode_s *node = tree->root;
	iBiNode_s *nextrt = NULL;

	if (key == 0) return ibi_first(tree);
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

#if 0
/* Rotations:
 *     y    right->   x
 *    / \   <-left   / \
 *   x   c          a   y
 *  / \                / \
 * a   b              b   c
 */
static void rot_right (iBiNode_s **np)
{
	iBiNode_s *x;
	iBiNode_s *y;
	iBiNode_s *tmp;
	y = *np;
	if (!y) return;
	x = y->left;
	if (!x) return;
	tmp = x->right;
	x->right = y;
	y->left = tmp;
	*np = x;
}

static void rot_left (iBiNode_s **np)
{
	iBiNode_s *x;
	iBiNode_s *y;
	iBiNode_s *tmp;
	x = *np;
	if (!x) return;
	y = x->right;
	if (!y) return;
	tmp = y->left;
	y->left = x;
	x->right = tmp;
	*np = y;
}
#endif

static iBiNode_s *ibi_new (u64 key)
{
	iBiNode_s *node = ezalloc(sizeof(*node));
	node->key = key;
	return node;
}


int ibi_insert (iBiTree_s *tree, u64 key)
{
	iBiNode_s **np = &tree->root;
	for (;;) {
		iBiNode_s *node = *np;
		if (!node) break;
		if (key < node->key) {
			np = &node->left;
		} else {
			np = &node->right;
		}
	}
	*np = ibi_new(key);
	return 0;
}

static void delete_node (iBiNode_s **dp)
{
	//static int odd = 0;
	iBiNode_s **np = dp;
	iBiNode_s *deletee;
	iBiNode_s *node;
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
#if 0
	for (;;) {
		node = *np;
		if (!node->right) {
			*np = node->left;
			break;
		}
		if (!node->left) {
			*np = node->right;
			break;
		}
		if (odd & 1) {
			rot_left(np);
			np = &((*np)->left);
		} else {
			rot_right(np);
			np = &((*np)->right);
		}
		++odd;
	}
	free(node);
#endif
}

int ibi_delete (iBiTree_s *tree, u64 key)
{
	iBiNode_s **np = &tree->root;
	for (;;) {
		iBiNode_s *node = *np;
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
