/*
 * By Julienne Walker
 * License: Public Domain
 * url: http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
 */ 

#ifndef _JSWTREE_H_
#define _JSWTREE_H_

struct jsw_node {
	int red;
	int data;
	struct jsw_node *link[2];
};

struct jsw_tree {
	struct jsw_node *root;
};

int jsw_rb_assert (struct jsw_node *root);
int jsw_insert (struct jsw_tree *tree, int data);
int jsw_remove (struct jsw_tree *tree, int data);
#endif
