/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <eprintf.h>
#include <style.h>

#include "bitree.h"

struct BiNode_s {
	BiNode_s *left;
	BiNode_s *right;
	Rec_s rec;
};

int bi_audit (BiTree_s *tree)
{
	return 0;
}

static void pr_indent (int indent)
{
	while (indent-- > 0) {
		printf("    ");
	}
}

static void pr_lump (Lump_s a)
{
	enum { MAX_PR_LUMP = 32 };
	u8 *p;
	int size = a.size;
	if (size > MAX_PR_LUMP) size = MAX_PR_LUMP;
	for (p = a.d; size != 0; --size, p++) {
		if (isprint(*p)) {
			putchar(*p);
		} else {
			putchar('.');
		}
	}
}

static void pr_rec (Rec_s r, int indent)
{
	pr_indent(indent);
	pr_lump(r.key);
	printf(": ");
	pr_lump(r.val);
	printf("\n");
}

static void pr_node (BiNode_s *node, int indent)
{
	if (!node) return;
	pr_node(node->left, indent + 1);
	pr_rec(node->rec, indent);
	pr_node(node->right, indent + 1);
}

int bi_print (BiTree_s *tree)
{
	pr_node(tree->root, 0);
	return 0;
}

void bi_pr_path (BiTree_s *tree, Lump_s key)
{
	BiNode_s *node = tree->root;
	while (node) {
		pr_lump(node->rec.key);
		int r = cmplump(key, node->rec.key);
		if (r == 0) {
			printf("\n");
			return;
		}
		printf(" ");
		if (r < 0) {
			node = node->left;
		} else {
			node = node->right;
		}
	}
}

static void node_stat (BiNode_s *node, BiStat_s *s, int depth)
{
	if (!node) return;
	++s->num_nodes;
	if (depth > s->max_depth) {
		s->max_depth = depth;
		s->deepest = node->rec.key;
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
	
BiStat_s bi_stats (BiTree_s *tree)
{
	BiStat_s stat = { 0 };
	node_stat(tree->root, &stat, 1);
	return stat;
}

int bi_find (BiTree_s *tree, Lump_s key, Lump_s *val)
{
	BiNode_s *node = tree->root;
	while (node) {
		int r = cmplump(key, node->rec.key);
		if (r == 0) {
			*val = node->rec.val;
			return 0;
		}
		if (r < 0) {
			node = node->left;
		} else {
			node = node->right;
		}
	}
	return -1;
}

/* Rotations:
				y    right->   x
			/ \   <-left   / \
			x   c          a   y
		/ \                / \
		a   b              b   c
*/
static void rot_right (BiNode_s **np)
{
	BiNode_s *x;
	BiNode_s *y;
	BiNode_s *tmp;
	y = *np;
	if (!y) return;
	x = y->left;
	if (!x) return;
	tmp = x->right;
	x->right = y;
	y->left = tmp;
	*np = x;
}

static void rot_left (BiNode_s **np)
{
	BiNode_s *x;
	BiNode_s *y;
	BiNode_s *tmp;
	x = *np;
	if (!x) return;
	y = x->right;
	if (!y) return;
	tmp = y->left;
	y->left = x;
	x->right = tmp;
	*np = y;
}

static BiNode_s *bi_new (Rec_s r)
{
	BiNode_s *node = ezalloc(sizeof(*node));
	node->rec = r;
	return node;
}

#if 0
static void insert (BiNode_s **np, Rec_s r)
{
	BiNode_s *node = *np;
	if (*np) {
		if (cmplump(r.key, node->rec.key) < 0) {
			insert(&node->left, r);
		} else {
			insert(&node->right, r);
		}
	} else {
		*np = bi_new(r);
	}
}

int bi_insert (BiNode_s **root, Rec_s r)
{
	insert(&t->root, r);
	return 0;
}

static void delete_node (BiNode_s **np)
{
	BiNode_s *node = *np;
	if (!node->right) {
		*np = node->left;
		free(node);
		return;
	}
	if (!node->left) {
		*np = node->right;
		free(node);
		return;
	}
	rot_right(np);
	delete_node(&((*np)->right));
}

static void delete (BiNode_s **np, Lump_s key)
{
	BiNode_s *node = *np;
	if (!node) fatal("Key not found");
	int r = cmplump(key, node->rec.key);
	if (r < 0) {
		delete(&node->left, key);
	} else if (r > 0) {
		delete(&node->right, key);
	} else {
		delete_node(np);
	}
}

int bi_delete (BiNode_s **root, Lump_s key)
{
	delete(&t->root, key);
	return 0;
}
#endif

int bi_insert (BiTree_s *tree, Rec_s r)
{
	BiNode_s **np = &tree->root;
	for (;;) {
		BiNode_s *node = *np;
		if (!node) break;
		if (cmplump(r.key, node->rec.key) < 0) {
			np = &node->left;
		} else {
			np = &node->right;
		}
	}
	*np = bi_new(r);
	return 0;
}

static void delete_node (BiNode_s **np)
{
	static int odd = 0;
	BiNode_s *node;
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

int bi_delete (BiTree_s *tree, Lump_s key)
{
	BiNode_s **np = &tree->root;
	for (;;) {
		BiNode_s *node = *np;
		if (!node) fatal("Key not found");
		int r = cmplump(key, node->rec.key);
		if (r == 0) break;
		if (r < 0) {
			np = &node->left;
		} else {
			np = &node->right;
		}
	}
	delete_node(np);
	return 0;
}
