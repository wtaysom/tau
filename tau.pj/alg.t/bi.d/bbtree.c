/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

/* Binary B Tree */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "bbtree.h"

#define PRk(_key) prc(FN_ARG, # _key, '@' + _key)
#define PRnode(_n) printf("%s<%d> %s %p %d %c %p %p\n",		\
	__FUNCTION__, __LINE__, # _n, (_n), (_n)->siblings,	\
	(char)(_n)->key + '@', (_n)->left, (_n)->right)

struct BbNode_s {
	BbNode_s *left;
	BbNode_s *right;
	int siblings;
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
	pr_node(node->right, indent + 1);

	pr_indent(indent);
	if (node->key < 26) {
		printf("%d %c\n", node->siblings, '@' + (u8)node->key);
	} else {
		printf("%d %lld\n", node->siblings, node->key);
	}

	pr_node(node->left, indent + 1);
}

int bb_print (BbTree_s *tree)
{
	puts("----------------\n");
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
 *     y    right->   x
 *    / \   <-left   / \
 *   x   c          a   y
 *  / \                / \
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

/*
 *          gp
 *          |
 *          h-i
 *         /
 *        b-d-f
 *       / / / \
 *      a c e   g
 *
 *           gp
 *           |
 *           d-----h-----i
 *          /     /
 *         /     /
 *        b     f
 *       / \   / \
 *      a   c e   g
 */
/*
 * Three possible cases for split, all slightly different.
 *          gp
 *          |
 *    head->h---------q-----\
 *         /         /       \
 *        b-d-f     k-m-o     s-u-w
 *       / / / \   / / / \   / / / \
 *      a c e   g j l n   p r t v   x
 *
 *          gp
 *          |
 *          d-----h-----m-----q------u
 *         /     /     /     /      / \
 *        /     /     /     /      /   \
 *       b     f     k     o      s     w
 *      / \   / \   / \   / \    / \   / \
 *     a   c e   g j   l n   p  r   t v   x
 */
BbNode_s **split (BbNode_s *head, BbNode_s **pp, BbNode_s *node)
{
	BbNode_s *bravo  = node;
	BbNode_s *delta  = bravo->right;
	BbNode_s *parent = *pp;
#if 0
PRk(head->key);
PRk(node->key);
PRk(parent->key);
PRk(bravo->key);
PRk(delta->key);
#else
PRnode(head);
PRnode(node);
PRnode(parent);
PRnode(bravo);
PRnode(delta);
#endif
	bravo->right = delta->left;
	delta->left = bravo;
	bravo->siblings = 0;
	delta->siblings = 0;

	if (parent->siblings) {
HERE;
		parent->left = delta->right;
		delta->right = parent;
		*pp = delta;
		delta->siblings = parent->siblings + 1;
		parent->siblings = 0;
	} else if (delta->key < parent->key) {
HERE;
		parent->left = delta->right;
		delta->right = parent;
		*pp = delta;
		++head->siblings;
	} else {
HERE;
		parent->right = delta;
		++head->siblings;  // Wrong thing to increment
PRnode(head);
	}
	return pp;
}

/*
 *        r
 *        |
 *        b-d-f
 *       / / / \
 *      a c e   g
 *
 *           r
 *           |
 *           d
 *          / \
 *         /   \
 *        b     f
 *       / \   / \
 *      a   c e   g
 */
static void grow (BbNode_s **rootp)
{
	BbNode_s *bravo = *rootp;
	BbNode_s *delta = bravo->right;

	bravo->right = delta->left;
	delta->left = bravo;
	*rootp = delta;
	bravo->siblings = 0;
	delta->siblings = 0;
}
	
void bb_insert (BbTree_s *tree, u64 key)
{
	BbNode_s **np = &tree->root;
	BbNode_s **pp;
	BbNode_s *head = NULL;
	BbNode_s *node = *np;
	int siblings;

	if (!node) {
		*np = bb_new(key);
		return;
	}
	if (node->siblings == 2) {
		grow(np);
		node = *np;
	}
	head = node;
	siblings = head->siblings;
	while (node->left) {
		pp = np;
		if (key < node->key) {
			np = &node->left;
			node = *np;
			head = node;
			siblings = head->siblings;
		} else {
			np = &node->right;
			node =*np;
// Need to know when going right goes down a level.
// 1. keep a count from head
// 2. Keep a count of right siblings
			if (siblings) {
				--siblings;
			} else {
				head = node;
				siblings = head->siblings;
			}
			assert(node);
		}
		if (node->siblings == 2) {
			np = split(head, pp, node);
			node = *np;
		}
		assert(node->siblings < 2);
	}
	if (!node->left) {
		if (key < node->key) {
			BbNode_s *child = bb_new(key);
			child->right = node;
			child->siblings = node->siblings + 1;
			node->siblings = 0;
			*np = child;
			return;
		}
		head = node;
		for (;;) {
			np = &node->right;
			node = *np;
			if (!node) {
				*np = bb_new(key);
				++head->siblings;
				return;
			}
			if (key < node->key) {
				BbNode_s *child = bb_new(key);
				child->right = node;
				child->siblings = node->siblings + 1;
				node->siblings = 0;
				*np = child;
				++head->siblings;
				return;
			}
		}
	}
#if 0
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
#endif
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

void bb_delete (BbTree_s *tree, u64 key)
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
}
