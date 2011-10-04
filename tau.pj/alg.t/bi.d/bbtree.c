/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

/* Binary B Tree */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <eprintf.h>
#include <style.h>

#include "bbtree.h"

struct BbNode_s {
	BbNode_s *left;
	BbNode_s *right;
	int num_siblings;
	u64 key;
};

int bb_audit (BbTree_s *tree)
{
	return 0;
}

static void pr_indent (int indent)
{
	while (indent-- > 0) {
		printf("    ");
	}
}

static void pr_node (BbNode_s *node, int indent)
{
	if (!node) return;
	pr_node(node->left, indent + 1);

	pr_indent(indent);
	printf("%d %lld\n", node->num_siblings, node->key);

	pr_node(node->right, indent + 1);
}

int bb_print (BbTree_s *tree)
{
	pr_node(tree->root, 0);
	return 0;
}

void bb_pr_path (BbTree_s *tree, u64 key)
{
	BbNode_s *node = tree->root;
	while (node) {
		printf("%lld", node->key);
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

static void node_stat (BbNode_s *node, BbStat_s *s, int depth)
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
	
BbStat_s bb_stats (BbTree_s *tree)
{
	BbStat_s stat = { 0 };
	node_stat(tree->root, &stat, 1);
	return stat;
}

int bb_find (BbTree_s *tree, u64 key)
{
	BbNode_s *node = tree->root;
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

/* Rotations:
 *   y    right->   x
 *  / \   <-left   / \
 *  x   c          a   y
 * / \                / \
 * a   b              b   c
 */
static void rot_right (BbNode_s **np)
{
	BbNode_s *x;
	BbNode_s *y;
	BbNode_s *tmp;
	y = *np;
	if (!y) return;
	x = y->left;
	if (!x) return;
	tmp = x->right;
	x->right = y;
	y->left = tmp;
	*np = x;
}

static void rot_left (BbNode_s **np)
{
	BbNode_s *x;
	BbNode_s *y;
	BbNode_s *tmp;
	x = *np;
	if (!x) return;
	y = x->right;
	if (!y) return;
	tmp = y->left;
	y->left = x;
	x->right = tmp;
	*np = y;
}

static BbNode_s *bb_new (u64 key)
{
	BbNode_s *node = ezalloc(sizeof(*node));
	node->key = key;
	return node;
}

int bb_insert (BbTree_s *tree, u64 key)
{
	BbNode_s **np = &tree->root;
	for (;;) {
		BbNode_s *node = *np;
		if (!node) break;
		if (key < node->key) {
			np = &node->left;
		} else {
			np = &node->right;
		}
	}
	*np = bb_new(key);
	return 0;
}

static void delete_node (BbNode_s **np)
{
	static int odd = 0;
	BbNode_s *node;
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
}

int bb_delete (BbTree_s *tree, u64 key)
{
	BbNode_s **np = &tree->root;
	for (;;) {
		BbNode_s *node = *np;
		if (!node) fatal("Key not found");
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
