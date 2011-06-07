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
 * Also see: Sedgweick, Robert, "Algorithms, 2nd Ed."
 *		Addison-Wesley, 1988, pp 215-230.
 *
 * Read-Black Tree Rules:
 *	1. Every node is either red or black
 *	2. The root is black
 *	3. Every nil leaf is black
 *	4. If a node is red, the both its children are black
 *	5. For each node, all paths form the node to descendant
 *		leaves contain the same number of black nodes.
 *
 * I use the prefix "RBP_" to indicate Red-Black tree with a Parent pointer.
 * RBP - for functions to be called by user.
 * rbp - for internal functions used by algorithm
 * rbp - to prefix field names.
 *
 * The RBP_Tree_s structure defines the type of the red-black tree.  This
 * lets one tree type be associated with multiple roots.  This means that
 * both the tree (type) and the root are passed to the tree manipulation
 * routines.
 *
 * To make life simpler, if the root of the tree is set to NULL, the code
 * will handle the real initialization of the tree.  Also, if the root is
 * ever set back to NIL, the code changes it to NULL.  This allows for a
 * simple test to check if the tree is empty.
 *
 * SMP Issue:  Because the nil node in the RBP_Tree_s structure can be
 * modified temporarily during delete operations, you must have separate
 * RBP_Tree_s for each tree that can be updated independently.
 *
 *
 * RBP_Init		Initialize a red-black tree.
 *
 *	tree:		Pointer to type of tree being initialized
 *
 *	nodeCompare:	Compare two nodes.  The node pointers have been
 *			adjusted to point to the beginning of the structure.
 *			If the first node is less then the second node, return
 *			a negative number.  If equal, return 0.  If greater
 *			than, return a positive number.
 *
 *	keyCompare:	Compare a node to a key.  Note: a pointer to the key
 *			is passed.
 *
 *	destroyNode:	Routine to destroy the type of node stored in this tree.
 *
 *	dumpNode:	A routine to print the contents of a node.  Used by
 *			debugging and auditing routines.
 *
 *	offset:		Because the user supplies the nodes imbedded in
 *			structures, this give the offset into the structure
 *			for the node.  The above functions are called with
 *			structure pointer at the beginning of the structure.
 *
 *
 * RBP_Insert		Insert a node into the tree.  Because we know the offset
 *			of the red-black node structure, you just pass in a
 *			pointer to the beginning of the user structure.
 *
 *	tree:		Pointer to tree type.
 *
 *	proot:		Pointer to a pointer to the root node for tree.  The
 *			insert operation can change which node is at the root
 *			of the tree.
 *
 *	node:		User node to inserted to the tree.  All key fields must
 *			be filled in.  Note: node is a pointer to the beginning
 *			of the sturcture, not the RBP_Node_s field in the
 *			structure.
 *
 *
 */

#ifndef _RBPTREE_H_
#define _RBPTREE_H_

#ifndef _STYLE_H_
#include <style.h>
#endif

#define RBP_DEBUG DISABLE

typedef struct RBP_Node_s	RBP_Node_s;

struct RBP_Node_s {
	RBP_Node_s	*p;	/* Parent of node	*/
	RBP_Node_s	*l;	/* Left child		*/
	RBP_Node_s	*r;	/* Right child		*/
	bool		red;	/* Color		*/
};

typedef struct RBP_Tree_s
{
	unint	rbpOffset;	/* Offset of node in structure	*/

	snint	(*rbpNodeCompare)(	/* x <  y --> -	*/
			void *x,	/* x == y --> 0	*/
			void *y);	/* x >  y --> +	*/

	snint	(*rbpKeyCompare)(	/* key <  x.key --> -	*/
			void *key,	/* key == x.key --> 0	*/
			void *x);	/* key >  x.key --> +	*/

	bool	(*rbpDestroyNode)(
			void *x);

	bool	(*rbpDumpNode)(		/* Print the given node	*/
			void *x,
			unint depth,
			addr msg);	/* Why node is being dumped	*/

	RBP_Node_s	rbpNil;

#if RBP_DEBUG IS_ENABLED
	unint	rbpInserts;
	unint	rbpDeletes;
	unint	rbpLeftRotates;
	unint	rbpRightRotates;
#endif

} RBP_Tree_s;

	/*
	 * Audits can be used to debug and track down errors in red-black trees,
	 */
enum { MAX_DEPTH = 32 };

typedef struct RBP_Audit_s
{
	unint	nodes;
	void	*maxNode;
	unint	both;
	unint	right;
	unint	left;
	unint	none;
	unint	veryDeep;
	unint	maxDepth;
	unint	redNodes;
	unint	blackNodes;
	unint	depth[MAX_DEPTH];
	unint	red[MAX_DEPTH];
	unint	black[MAX_DEPTH];
} RBP_Audit_s;

#define RBP_INIT_TREE(_tree, _nodeCompare, _keyCompare, _destroyNode,	\
			_dumpNode, _offset)				\
{									\
	(_offset), (_nodeCompare), (_keyCompare), (_destroyNode), (_dumpNode),\
	{ &(_tree).rbpNil, &(_tree).rbpNil, &(_tree).rbpNil, FALSE }	\
}

void RBP_Init(
	RBP_Tree_s	*tree,
	snint		(*nodeCompare)(void *x, void *y),
	snint		(*keyCompare)(void *key, void *x),
	bool		(*destroyNode)(void *x),
	bool		(*dumpNode)(void *x, unint depth, addr msg),
	unint		offset);

void RBP_Insert(RBP_Tree_s *tree, RBP_Node_s **proot, void *node);
void RBP_Delete(RBP_Tree_s *tree, RBP_Node_s **proot, void *node);
void *RBP_Find(RBP_Tree_s *tree, RBP_Node_s *root, void *key);
void *RBP_FindCeiling(RBP_Tree_s *tree, RBP_Node_s *root, void *key);
void *RBP_FindFloor(RBP_Tree_s *tree, RBP_Node_s *root, void *key);

void *RBP_Min(RBP_Tree_s *tree, RBP_Node_s *root);
void *RBP_Max(RBP_Tree_s *tree, RBP_Node_s *root);
void *RBP_Succ(RBP_Tree_s *tree, void *node);
void *RBP_Pred(RBP_Tree_s *tree, void *node);

void RBP_Audit(RBP_Tree_s *tree, RBP_Node_s *root, RBP_Audit_s *audit);

void RBP_InOrder(
	RBP_Tree_s	*tree,
	RBP_Node_s	*root,
	bool		(*userFunc)(
				void	*node,
				unint	depth,
				addr	userData),
	addr		userData);

void RBP_PostOrder(
	RBP_Tree_s	*tree,
	RBP_Node_s	*root,
	bool		(*userFunc)(
				void	*node,
				unint	depth,
				addr	userData),
	addr		userData);

void RBP_DestroyTree(RBP_Tree_s *tree, RBP_Node_s **proot);

#endif
