/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

/*
 * Red-Black Tree Algorithm
 *
 * Cormen, Thomas H. [et al], "Introduction to Algorithms, 2nd Ed."
 *		MIT Press, McGraw-Hill, 2001, pp 273-301.
 *
 * See rbpTree.h for more documentation on red-black trees.
 */

#include <string.h>
#include "rbpTree.h"

enum { BLACK, RED };

#if RBP_DEBUG IS_ENABLED
#define INC(_x)	(++(_x))
#else
#define INC(_x)	((void)0)
#endif


#define NODE(_x)	((_x == NIL)					\
				? NULL					\
				: ((void *)(((addr)(_x)) - tree->rbpOffset)))

#define RBP_NODE(_x)		((RBP_Node_s *)(((addr)(_x)) + tree->rbpOffset))

#define COMPARE_NODES(_x, _y)	(tree->rbpNodeCompare(NODE(_x), NODE(_y)))

#define COMPARE_KEY(_k, _x)	(tree->rbpKeyCompare((_k), NODE(_x)))

#define NIL	(&tree->rbpNil)

void RBP_Init (
	RBP_Tree_s	*tree,
	snint		(*nodeCompare)(void *x, void *y),	
	snint		(*keyCompare)(void *key, void *x),
	bool		(*destroyNode)(void *x),
	bool		(*dumpNode)(void *x, unint depth, addr msg),
	unint		offset)
{
	bzero(tree, sizeof(*tree));
	tree->rbpNodeCompare = nodeCompare;
	tree->rbpKeyCompare  = keyCompare;
	tree->rbpDestroyNode = destroyNode;
	tree->rbpDumpNode    = dumpNode;
	tree->rbpOffset      = offset;

	tree->rbpNil.p    = NIL;
	tree->rbpNil.l    = NIL;
	tree->rbpNil.r    = NIL;
	tree->rbpNil.red  = BLACK;
}

RBP_Node_s *rbpMin (RBP_Tree_s *tree, RBP_Node_s *x)
{
	while (x->l != NIL) {
		x = x->l;
	}
	return x;
}

RBP_Node_s *rbpMax (RBP_Tree_s *tree, RBP_Node_s *x)
{
	while (x->r != NIL) {
		x = x->r;
	}
	return x;
}

RBP_Node_s *rbpSucc (RBP_Tree_s *tree, RBP_Node_s *x)
{
	RBP_Node_s	*y;

	if (x->r != NIL) {
		return rbpMin(tree, x->r);
	}
	y = x->p;
	while ((y != NIL) && (x == y->r)) {
		x = y;
		y = y->p;
	}
	return y;
}

RBP_Node_s *rbpPred (RBP_Tree_s *tree, RBP_Node_s *x)
{
	RBP_Node_s	*y;

	if (x->l != NIL) {
		return rbpMax(tree, x->l);
	}
	y = x->p;
	while ((y != NIL) && (x == y->l)) {
		x = y;
		y = y->p;
	}
	return y;
}

/*
 *      d                   b
 *     / \  rotate right   / \
 *    b   e ------------> a   d
 *   / \    <-----------     / \
 *  a   c    rotate left    c   e
 */
void rbpLeftRotate (RBP_Tree_s *tree, RBP_Node_s **proot, RBP_Node_s *x)
{
	RBP_Node_s	*y;

	INC(tree->rbpLeftRotates);

	y = x->r;
	x->r = y->l;
	if (y->l != NIL) {
		y->l->p = x;
	}
	y->p = x->p;
	if (x->p == NIL) {
		*proot = y;
	} else {
		if (x == x->p->l) {
			x->p->l = y;
		} else {
			x->p->r = y;
		}
	}
	y->l = x;
	x->p = y;
}

void rbpRightRotate (RBP_Tree_s *tree, RBP_Node_s **proot, RBP_Node_s *x)
{
	RBP_Node_s	*y;

	INC(tree->rbpRightRotates);

	y = x->l;
	x->l = y->r;
	if (y->r != NIL) {
		y->r->p = x;
	}
	y->p = x->p;
	if (x->p == NIL) {
		*proot = y;
	} else {
		if (x == x->p->r) {
			x->p->r = y;
		} else {
			x->p->l = y;
		}
	}
	y->r = x;
	x->p = y;
}

void rbpInsertFixup (RBP_Tree_s *tree, RBP_Node_s **proot, RBP_Node_s *z)
{
	RBP_Node_s	*y;

	while (z->p->red) {
		if (z->p == z->p->p->l) {
			y = z->p->p->r;
			if (y->red) {
				z->p->red = BLACK;
				y->red = BLACK;
				z->p->p->red = RED;
				z = z->p->p;
			} else {
				if (z == z->p->r) {
					z = z->p;
					rbpLeftRotate(tree, proot, z);
				}
				z->p->red = BLACK;
				z->p->p->red = RED;
				rbpRightRotate(tree, proot, z->p->p);
			}
		} else {
			y = z->p->p->l;
			if (y->red) {
				z->p->red = BLACK;
				y->red = BLACK;
				z->p->p->red = RED;
				z = z->p->p;
			} else {
				if (z == z->p->l) {
					z = z->p;
					rbpRightRotate(tree, proot, z);
				}
				z->p->red = BLACK;
				z->p->p->red = RED;
				rbpLeftRotate(tree, proot, z->p->p);
			}
		}
	}
	(*proot)->red = BLACK;
}

void RBP_Insert (RBP_Tree_s *tree, RBP_Node_s **proot, void *node)
{
	RBP_Node_s	*z = RBP_NODE(node);
	RBP_Node_s	*y, *x;

	INC(tree->rbpInserts);

	y = NIL;
	x = *proot;
	if (x == NULL) {
		*proot = x = NIL;	/* Initialize the root */
	}
	while (x != NIL) {
		y = x;
		if (COMPARE_NODES(z, x) < 0) {
			x = x->l;
		} else {
			x = x->r;
		}
	}
	z->p = y;
	if (y == NIL) {
		*proot = z;
	} else {
		if (COMPARE_NODES(z, y) < 0) {
			y->l = z;
		} else {
			y->r = z;
		}
	}
	z->l = z->r = NIL;
	z->red = RED;
	rbpInsertFixup(tree, proot, z);
}

void rbpDeleteFixup (RBP_Tree_s *tree, RBP_Node_s **proot, RBP_Node_s *x)
{
	RBP_Node_s	*w;

	while ((x != *proot) && (!x->red)) {
		if (x == x->p->l) {
			w = x->p->r;
			if (w->red) {
				w->red = BLACK;
				x->p->red = RED;
				rbpLeftRotate(tree, proot, x->p);
				w = x->p->r;
			}
			if (!w->l->red && !w->r->red) {
				w->red = RED;
				x = x->p;
			} else {
				if (!w->r->red) {
					w->l->red = BLACK;
					w->red = RED;
					rbpRightRotate(tree, proot, w);
					w = x->p->r;
				}
				w->red = x->p->red;
				x->p->red = BLACK;
				w->r->red = BLACK;
				rbpLeftRotate(tree, proot, x->p);
				x = *proot;
			}
		} else {
			w = x->p->l;
			if (w->red) {
				w->red = BLACK;
				x->p->red = RED;
				rbpRightRotate(tree, proot, x->p);
				w = x->p->l;
			}
			if (!w->r->red && !w->l->red) {
				w->red = RED;
				x = x->p;
			} else {
				if (!w->l->red) {
					w->r->red = BLACK;
					w->red = RED;
					rbpLeftRotate(tree, proot, w);
					w = x->p->l;
				}
				w->red = x->p->red;
				x->p->red = BLACK;
				w->l->red = BLACK;
				rbpRightRotate(tree, proot, x->p);
				x = *proot;
			}
		}
	}
	x->red = BLACK;
}

void RBP_Delete (RBP_Tree_s *tree, RBP_Node_s **proot, void *node)
{
	RBP_Node_s	*z = RBP_NODE(node);
	RBP_Node_s	*y, *x;

	INC(tree->rbpDeletes);

	if ((z->l == NIL) || (z->r == NIL)) {
		y = z;
	} else {
		y = rbpSucc(tree, z);
	}
	if (y->l == NIL) {
		x = y->r;
	} else {
		x = y->l;
	}
	x->p = y->p;
	if (y->p == NIL) {
		*proot = x;
	} else {
		if (y == y->p->l) {
			y->p->l = x;
		} else {
			y->p->r = x;
		}
	}
	if (!y->red) {
		rbpDeleteFixup(tree, proot, x);
	}
	if (y != z) {
		/* Switch pointers - splice y into z's place */
		y->red = z->red;
		y->r = z->r;
		y->l = z->l;
		y->p = z->p;
		if (y->r != NIL) y->r->p = y;
		if (y->l != NIL) y->l->p = y;
		if (y->p == NIL) {
			*proot = y;
		} else if (y->p->l == z) {
			y->p->l = y;
		} else {
			y->p->r = y;
		}
	}
	z->p = NULL;
	if (*proot == NIL) {
		*proot = NULL;
	}
}

void *RBP_Find (RBP_Tree_s *tree, RBP_Node_s *root, void *key)
{
	RBP_Node_s	*x;
	snint		compare;

	x = root;
	if (x == NULL) {
		return NULL;
	}
	while (x != NIL) {
		compare = COMPARE_KEY(key, x);
		if (compare == 0) {
			return NODE(x);
		}
		x = (compare < 0) ? x->l : x->r;
	}
	return NULL;
}

void *RBP_FindFloor (RBP_Tree_s *tree, RBP_Node_s *root, void *key)
{
	RBP_Node_s	*x;
	RBP_Node_s	*floor;
	snint		compare;

	floor = NIL;
	x = root;
	if (x == NULL) {
		return NULL;
	}
	while (x != NIL) {
		compare = COMPARE_KEY(key, x);
		if (compare == 0) {
			for (;;) {
				floor = x;
				x = rbpPred(tree, x);
				if (x == NIL) {
					break;
				}
				if (COMPARE_KEY(key, x) != 0) {
					break;
				}
			}
			return NODE(floor);
		}
		if (compare < 0) {
			x = x->l;
		} else {
			floor = x;
			x = x->r;
		}
	}
	return NODE(floor);
}

void *RBP_FindCeiling (RBP_Tree_s *tree, RBP_Node_s *root, void *key)
{
	RBP_Node_s	*x;
	RBP_Node_s	*ceiling;
	snint		compare;

	ceiling = NIL;
	x = root;
	if (x == NULL) {
		return NULL;
	}
	while (x != NIL) {
		compare = COMPARE_KEY(key, x);
		if (compare == 0) {
			for (;;) {
				ceiling = x;
				x = rbpSucc(tree, x);
				if (x == NIL) {
					break;
				}
				if (COMPARE_KEY(key, x) != 0) {
					break;
				}
			}
			return NODE(ceiling);
		}
		if (compare < 0) {
			ceiling = x;
			x = x->l;
		} else {
			x = x->r;
		}
	}
	return NODE(ceiling);
}

void *RBP_Min (RBP_Tree_s *tree, RBP_Node_s *root)
{
	RBP_Node_s	*x;

	if (root == NULL) {
		return NULL;
	}
	x = rbpMin(tree, root);
	return NODE(x);
}

void *RBP_Max (RBP_Tree_s *tree, RBP_Node_s *root)
{
	RBP_Node_s	*x;

	if (root == NULL) {
		return NULL;
	}
	x = rbpMax(tree, root);
	return NODE(x);
}

void *RBP_Succ (RBP_Tree_s *tree, void *node)
{
	RBP_Node_s	*x;

	if (node == NULL) {
		return NULL;
	}
	x = rbpSucc(tree, RBP_NODE(node));
	return NODE(x);
}

void *RBP_Pred (RBP_Tree_s *tree, void *node)
{
	RBP_Node_s	*x;

	if (node == NULL) {
		return NULL;
	}
	x = rbpPred(tree, RBP_NODE(node));
	return NODE(x);
}

#if 0
//	/* The recursive definition of an inorder traversal of the tree */
//bool rbpInorder (
//	RBP_Tree_s	*tree,
//	RBP_Node_s	*x,
//	unint		depth,
//	bool		(*userFunc)(
//					void	*node,
//					unint	depth,
//					addr	userData),
//	addr		userData)
//{
//	if (x == NIL) {
//		return TRUE;
//	}
//	if (!rbpInorder(tree, x->l, depth+1, userFunc, userData)) {
//		return FALSE;
//	}
//	if (!userFunc(NODE(x), depth, userData)) {
//		return FALSE;
//	}
//	if (!rbpInorder(tree, x->r, depth+1, userFunc, userData)) {
//		return FALSE;
//	}
//	return TRUE;
//}
//
//void RBP_Inorder (
//	RBP_Tree_s	*tree,
//	RBP_Node_s	*root,
//	bool		(*userFunc)(
//					void	*node,
//					unint	depth,
//					addr	userData),
//	addr		userData)
//{
//	rbpInorder(tree, root, 0, userFunc, userData);
//}
#endif

void RBP_InOrder (
	RBP_Tree_s	*tree,
	RBP_Node_s	*root,
	bool		(*userFunc)(
					void	*node,
					unint	depth,
					addr	userData),
	addr		userData)
{
	RBP_Node_s	*x = root;
	RBP_Node_s	*p;		/* Parent of x */
	unint		depth = 0;

	if (x == NULL) {
		return;
	}
	while (x->l != NIL) {
		x = x->l; ++depth;
	}
	while (x != NIL) {
		if (!userFunc(NODE(x), depth, userData)) {
			return;
		}
		if (x->r != NIL) {
			x = x->r; ++depth;
			while (x->l != NIL) {
				x = x->l; ++depth;
			}
			continue;
		}
		p = x->p;
		while ((p != NIL) && (x == p->r)) {
			x = p; --depth;
			p = p->p;
		}
		x = p; --depth;
	}
}

void RBP_PostOrder (
	RBP_Tree_s	*tree,
	RBP_Node_s	*root,
	bool		(*userFunc)(
					void	*node,
					unint	depth,
					addr	userData),
	addr		userData)
{
	RBP_Node_s	*x = root;
	RBP_Node_s	*p;		/* Parent of x */
	unint		depth = 0;

	if (x == NULL) {
		return;
	}
	while (x->l != NIL) {
		x = x->l; ++depth;
	}
	while (x != NIL) {
		if (x->r != NIL) {
			x = x->r; ++depth;
			while (x->l != NIL) {
				x = x->l; ++depth;
			}
			continue;
		}
		p = x->p;
		while ((p != NIL) && (x == p->r)) {
			if (!userFunc(NODE(x), depth, userData)) {
				return;
			}
			x = p; --depth;
			p = p->p;
		}
		if (!userFunc(NODE(x), depth, userData)) {
			return;
		}
		x = p; --depth;
	}
}

void RBP_DestroyTree (RBP_Tree_s *tree, RBP_Node_s **proot)
{
	RBP_Node_s	*x = *proot;

	*proot = NULL;

	RBP_PostOrder(tree, x, (bool (*)(void *, unint, addr))tree->rbpDestroyNode, 0);
}

void rbpDumpNode (RBP_Tree_s *tree, RBP_Node_s *x, char *msg)
{
	tree->rbpDumpNode(NODE(x), 0, (addr)msg);
}

static void auditNode (
	RBP_Audit_s	*audit,
	RBP_Tree_s	*tree,
	RBP_Node_s	*x,
	int			depth,
	int			red,
	int			black,
	int			lastred)
{
	if (x == NIL) {
		if (depth > audit->maxDepth) {
			audit->maxDepth = depth;
		}
		if (depth < MAX_DEPTH) {
			++audit->depth[depth];
			++audit->red[red];
			++audit->black[black];
		} else {
			++audit->veryDeep;
		}
		return;
	}
	++audit->nodes;
	if ((x->l != NIL) && (x->r != NIL)) {
		++audit->both;
	} else if (x->l != NIL) {
		++audit->left;
	} else if (x->r != NIL) {
		++audit->right;
	} else {
		++audit->none;
	}
	if (x->red) {
		++audit->redNodes;
		++red;
	} else {
		++audit->blackNodes;
		++black;
	}

	if (lastred && x->red) {
		rbpDumpNode(tree, x, "Red followed by red");
	}
	lastred = x->red;

	auditNode(audit, tree, x->l, depth+1, red, black, lastred);

	if (audit->maxNode == NULL) {
		audit->maxNode = x;
	} else {
		if (COMPARE_NODES(x, audit->maxNode) < 0) {
			rbpDumpNode(tree, x, "Key out of order");
			rbpDumpNode(tree, audit->maxNode, "Max key");
		} else {
			audit->maxNode = x;
		}
	}
	auditNode(audit, tree, x->r, depth+1, red, black, lastred);
}

void RBP_Audit (RBP_Tree_s *tree, RBP_Node_s *root, RBP_Audit_s *audit)
{
	bzero(audit, sizeof(*audit));
	auditNode(audit, tree, root, 0, 0, 0, 0);
}
