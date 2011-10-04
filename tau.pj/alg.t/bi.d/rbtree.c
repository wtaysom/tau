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

#include "rbtree.h"

enum { BLACK = 0, RED = 1 };

struct RbNode_s {
	addr left;
	addr right;
	u64 key;
};

#define set_red(_link)  ((_link) |= RED)
#define clr_red(_link)  ((_link) &= ~RED)
#define is_red(_link)   ((_link) & RED)
#define is_black(_link) (!is_red(_link))
#define ptr(_link)      ((RbNode_s *)((_link) & ~RED))

int rb_audit (RbTree_s *tree)
{
	return 0;
}

static void pr_indent (int indent)
{
	while (indent-- > 0) {
		printf("    ");
	}
}

static void pr_rec (addr link, int indent)
{
	RbNode_s *node = ptr(link);
	pr_indent(indent);
	printf("%c %llu\n", is_red(link) ? '*' : ' ',  node->key);
}

static void pr_node (addr link, int indent)
{
	RbNode_s *node = ptr(link);
	if (!node) return;
	pr_node(node->left, indent + 1);
	pr_rec(link, indent);
	pr_node(node->right, indent + 1);
}

int rb_print (RbTree_s *tree)
{
	printf("---\n");
	pr_node(tree->root, 0);
	return 0;
}

void rb_pr_path (RbTree_s *tree, u64 key)
{
	RbNode_s *node = ptr(tree->root);
	while (node) {
		printf("%lld", node->key);
		if (key == node->key) {
			printf("\n");
			return;
		}
		printf(" ");
		if (key < node->key) {
			node = ptr(node->left);
		} else {
			node = ptr(node->right);
		}
	}
}

static void node_stat (addr link, RbStat_s *s, int depth)
{
	RbNode_s *node = ptr(link);
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
	
RbStat_s rb_stats (RbTree_s *tree)
{
	RbStat_s stat = { 0 };
	node_stat(tree->root, &stat, 1);
	return stat;
}

int rb_find (RbTree_s *tree, u64 key)
{
	RbNode_s *node = ptr(tree->root);
	while (node) {
		if (key == node->key) {
			return 0;
		}
		if (key < node->key) {
			node = ptr(node->left);
		} else {
			node = ptr(node->right);
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
static void rot_right (addr *np)
{
	addr x;
	addr y;
	addr tmp;
	RbNode_s *xnode;
	RbNode_s *ynode;
	y = *np;
	if (!y) return;
	ynode = ptr(y);
	x = ynode->left;
	if (!x) return;
	xnode = ptr(x);
	tmp = xnode->right;
	xnode->right = y;
	ynode->left = tmp;
	*np = x;
}

static void rot_left (addr *np)
{
	addr x;
	addr y;
	addr tmp;
	RbNode_s *xnode;
	RbNode_s *ynode;
	x = *np;
	if (!x) return;
	xnode = ptr(x);
	y = xnode->right;
	if (!y) return;
	ynode = ptr(y);
	tmp = ynode->left;
	ynode->left = x;
	xnode->right = tmp;
	*np = y;
}

static addr rb_new (u64 key)
{
	RbNode_s *node = ezalloc(sizeof(*node));
	node->key = key;
	addr x = (addr)node;
	return set_red(x);
}

int rb_insert (RbTree_s *tree, u64 key)
{
	addr *np = &tree->root;
	addr *child;
	for (;;) {
		RbNode_s *node = ptr(*np);
		if (!node) break;
		if (key < node->key) {
			child = &node->left;
		} else {
			child = &node->right;
		}
		if (is_red(node->left)) {
			if (is_red(node->right)) {
				assert(is_black(*np));
				set_red(*np);
				clr_red(node->right);
				clr_red(node->left);
			} else {
				if (child == &node->left) {
					rot_right(np);
				}
			}
		} else {
			if (is_red(node->right)) {
				if (child == &node->right) {
					rot_left(np);
				}
			} else {
			}
		}
		np = child;
	}
	*np = rb_new(key);
	clr_red(tree->root);  // root is always black
	return 0;
}

static void delete_node (addr *np)
{
	static int odd = 0;
	RbNode_s *node;
	for (;;) {
		node = ptr(*np);
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
			np = &(ptr(*np)->left);
		} else {
			rot_right(np);
			np = &(ptr(*np)->right);
		}
		++odd;
	}
	free(node);
}

int rb_delete (RbTree_s *tree, u64 key)
{
	addr *np = &tree->root;
	for (;;) {
		RbNode_s *node = ptr(*np);
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
