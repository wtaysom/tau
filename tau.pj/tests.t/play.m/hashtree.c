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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <style.h>
#include <eprintf.h>
#include <crc.h>
#include <myio.h>
#include <q.h>

enum {	BUCKET_SHIFT  = 3,
	NUM_BUCKETS   = 1 << BUCKET_SHIFT,
	BUCKET_MASK   = NUM_BUCKETS - 1,
	START_SHIFT   = 32 - BUCKET_SHIFT,
	MAX_IN_BUCKET = (NUM_BUCKETS * 90) / 100 };

enum { ROOT, BRANCH, LEAF };

enum { NOT_FULL, FULL };

u64	Adds;

typedef struct name_s	name_s;
typedef struct node_s	node_s;

struct name_s {
	name_s	*n_next;
	u32	n_hash;
	char	n_name[0];
};

struct node_s {
	unint	n_type;
	unint	n_num;
	node_s	*n_parent;
	void	*n_bucket[NUM_BUCKETS];
};

typedef struct tree_s {
	unint	t_type;
	node_s	*t_buckets;
} tree_s;

u32 hash (char *s)
{
	u32	crc;

	crc = hash_string_32(s);
	return crc; //(crc << 8) >> 1;
}

void init_tree (tree_s *t)
{
	node_s	*leaf;

	t->t_type = ROOT;

	leaf = ezalloc(sizeof(node_s));
	leaf->n_type = LEAF;
	leaf->n_parent = (node_s *)t;

	t->t_buckets = leaf;
}

unint find_buddy (node_s *node, unint x, unint *pbuddy)
{
	unint	i = 0;
	unint	size = 1;
	unint	buddy;

	for (i = 0; i < BUCKET_SHIFT; i++) {
		buddy = x ^ size;
		if (node->n_bucket[x] != node->n_bucket[buddy])	{
			*pbuddy = x;
			return size;
		}
		if (buddy < x) x = buddy;
		size <<= 1;
	}
	*pbuddy = 0;
	return size;
}

int split (node_s *parent, node_s *child, unint x)
{
	unint	size;
	unint	buddy;
	node_s	*new;

	size = find_buddy(child, x, &buddy);
	if (size == 1) return FULL;

	new = ezalloc(sizeof(node_s));

	return -1;
}

void split_leaf (node_s *leaf)
{
	node_s	*new;

	new = ezalloc(sizeof(node_s));
	new->n_type = LEAF;
	new->n_parent = leaf->n_parent;
}	

void insert_leaf (node_s *node, name_s *nm, u32 shift)
{
	name_s	**bucket;
	unint	h = (nm->n_hash >> shift) & BUCKET_MASK;

	bucket = (name_s **)&node->n_bucket[h];
	nm->n_next = *bucket;
	*bucket = nm;
	++node->n_num;
	if (node->n_num > MAX_IN_BUCKET) {
		//return FULL;
	}
}

void insert_branch (node_s *node, name_s *nm, u32 shift)
{
	node_s	*child;
	unint	h = (nm->n_hash >> shift) & BUCKET_MASK;

	child = node->n_bucket[h];
	shift -=  BUCKET_SHIFT;
	if (child->n_type == LEAF) {
		insert_leaf(child, nm, shift);
	} else if (child->n_type == BRANCH) {
		insert_branch(child, nm, shift);
	} else {
		eprintf("node %p type %ld", child, child->n_type);
	}
}

void add_name (tree_s *t, char *s)
{
	name_s	*nm;
	node_s	*node;

	++Adds;
	nm = ezalloc(sizeof(name_s) + strlen(s) + 1);
	strcpy(nm->n_name, s);
	nm->n_hash = hash(s);

	node = t->t_buckets;
	if (!node) {
		init_tree(t);
		node = t->t_buckets;
	}
	if (node->n_type == LEAF) {
		insert_leaf(node, nm, START_SHIFT);
	} else if (node->n_type == BRANCH) {
		insert_branch(node, nm, START_SHIFT);
	} else {
		eprintf("node %p type %ld", node, node->n_type);
	}
}

void pr_depth (unint depth)
{
	while (depth--) {
		printf("  ");
	}
}

void pr_name (name_s *nm, unint depth)
{
	pr_depth(depth);
	while (nm) {
		printf("%8x %s ", nm->n_hash, nm->n_name);
		nm = nm->n_next;
	}
	printf("\n");
}

void pr_leaf (node_s *node, unint depth)
{
	unint	i;

	for (i = 0; i < NUM_BUCKETS; i++) {
		pr_name(node->n_bucket[i], depth);
	}
}

void pr_node (node_s *node, unint depth)
{
	unint	i;

	if (!node) return;
	++depth;
	if (node->n_type == LEAF) {
		pr_leaf(node, depth);
		return;
	}
	for (i = 0; i < NUM_BUCKETS; i++) {
		pr_node(node->n_bucket[i], depth);
	}
}

void pr_tree (tree_s *t)
{
	pr_node(t->t_buckets, 0);
}

void save_file (char *path, void *tree)
{
	printf("%s\n", path);
	add_name(tree, path);
}

void usage (void)
{
	printf("Usage: %s <directory>\n",
		getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	tree_s	tree;

	setprogname(argv[0]);

	if (argc < 2) usage();

	init_tree( &tree);

	walk_tree(argv[1], save_file, &tree);

	printf("Adds = %lld\n", Adds);

	pr_tree( &tree);
	return 0;
}
