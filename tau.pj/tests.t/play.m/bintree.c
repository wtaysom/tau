/*
 * Copyright 2009 - Novell, Inc.
 */
#include <stdio.h>
#include <mylib.h>
#include <eprintf.h>
#include <debug.h>

typedef struct tree_s	tree_s;
struct tree_s {
	tree_s	*tr_parent;
	tree_s	*tr_left;
	tree_s	*tr_right;
	int	tr_mark;
	int	tr_val;
};

void pr_depth (int depth)
{
	while (depth--) {
		printf("    ");
	}
}

void pr_node (tree_s *node, int depth)
{
	if (!node) return;

	pr_node(node->tr_left, depth+1);
	pr_depth(depth);
	printf("%d %s\n", node->tr_val, node->tr_mark ? "TRUE" : "");
	pr_node(node->tr_right, depth+1);
}

void pr_tree (tree_s *root)
{
	pr_node(root, 0);
}

void add_tree (tree_s **proot, int x)
{
	tree_s	*node;
	tree_s	*t;

	node = ezalloc(sizeof(*t));
	node->tr_val = x;

	t = *proot;
	if (!t) {
		*proot = node;
		return;
	}
	for (;;) {
		if (x <= t->tr_val) {
			if (t->tr_left) {
				t = t->tr_left;
				continue;
			} else {
				t->tr_left = node;
				break;
			}
		} else {
			if (t->tr_right) {
				t = t->tr_right;
				continue;
			} else {
				t->tr_right = node;
				break;
			}
		}
	}
	node->tr_parent = t;
}

void process (tree_s *node, int depth, void *data)
{
	pr_depth(depth);
	printf("%d\n", node->tr_val);
}

void r_inorder (
	tree_s	*t,
	int	depth,
	void	(*fn)(tree_s *node, int depth, void *data),
	void	*data)
{
	if (!t) return;
	r_inorder(t->tr_left, depth+1, fn, data);
	fn(t, depth, data);
	r_inorder(t->tr_right, depth+1, fn, data);
}

void inorder (
	tree_s	*root,
	void	(*fn)(tree_s *node, int depth, void *data),
	void	*data)
{
	tree_s	*t = root;
	tree_s	*p = NULL;		// Parent;
	int	depth = -1;

	if (!t) return;

	for (;;) {
		if (t) {
			++depth;
			p = t;
			t = p->tr_left;
		} else {
			for (;;) {
				if (p->tr_mark) {
					p->tr_mark = FALSE;
				} else {
					fn(p, depth, data);
					p->tr_mark = TRUE;
					t = p->tr_right;
					break;
				}
				--depth;
				p = p->tr_parent;
				if (!p) return;
			}
		}
	}
}

void r_preorder (
	tree_s	*t,
	int	depth,
	void	(*fn)(tree_s *node, int depth, void *data),
	void	*data)
{
	if (!t) return;
	fn(t, depth, data);
	r_preorder(t->tr_left, depth+1, fn, data);
	r_preorder(t->tr_right, depth+1, fn, data);
}

void preorder (
	tree_s	*root,
	void	(*fn)(tree_s *node, int depth, void *data),
	void	*data)
{
	tree_s	*t = root;
	tree_s	*p = NULL;		// Parent;
	int	depth = -1;

	if (!t) return;

	for (;;) {
		if (t) {
			++depth;
			p = t;
			fn(p, depth, data);
			t = p->tr_left;
		} else {
			for (;;) {
				if (p->tr_mark) {
					p->tr_mark = FALSE;
				} else {
					p->tr_mark = TRUE;
					t = p->tr_right;
					break;
				}
				--depth;
				p = p->tr_parent;
				if (!p) return;
			}
		}
	}
}

void r_postorder (
	tree_s	*t,
	int	depth,
	void	(*fn)(tree_s *node, int depth, void *data),
	void	*data)
{
	if (!t) return;
	r_postorder(t->tr_left, depth+1, fn, data);
	r_postorder(t->tr_right, depth+1, fn, data);
	fn(t, depth, data);
}

void postorder (
	tree_s	*root,
	void	(*fn)(tree_s *node, int depth, void *data),
	void	*data)
{
	tree_s	*t = root;
	tree_s	*p = NULL;		// Parent;
	int	depth = -1;

	if (!t) return;

	for (;;) {
		if (t) {
			++depth;
			p = t;
			t = p->tr_left;
		} else {
			for (;;) {
				if (p->tr_mark) {
					p->tr_mark = FALSE;
					fn(p, depth, data);
				} else {
					p->tr_mark = TRUE;
					t = p->tr_right;
					break;
				}
				--depth;
				p = p->tr_parent;
				if (!p) return;
			}
		}
	}
}

int main (int argc, char *argv[])
{
	tree_s	*root = NULL;
	int	i;
	int	x;

	for (i = 0; i < 10; i++) {
		x = range(10);
		add_tree( &root, x);
	}
	pr_tree(root);
//	inorder(root, process, NULL);
//	r_inorder(root, 0, process, NULL);
//	preorder(root, process, NULL);
//	r_preorder(root, 0, process, NULL);
	postorder(root, process, NULL);
	r_postorder(root, 0, process, NULL);
	pr_tree(root);
	return 0;
}
