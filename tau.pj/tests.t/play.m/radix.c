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

enum {	BUCKET_SHIFT  = 4,
	NUM_BUCKETS   = 1 << BUCKET_SHIFT,
	BUCKET_MASK   = NUM_BUCKETS - 1,
	START_SHIFT   = 32 - BUCKET_SHIFT,
	MAX_IN_BUCKET = (NUM_BUCKETS * 90) / 100 };

enum { EMPTY, NODE, LEAF };

u64	Adds;
u64	Nodes;

typedef struct node_s	node_s;
typedef struct bucket_s	bucket_s;

struct bucket_s {
	unint	b_type;
	union {
		unint	b_num;
		node_s	*b_node;
	};
};

struct node_s {
	bucket_s	n_bucket[NUM_BUCKETS];
};

typedef struct tree_s {
	bucket_s	t_root;
} tree_s;

void init_tree (tree_s *t)
{
	node_s	*node;

	++Nodes;
	node = ezalloc(sizeof(node_s)); //this isn't needed
	t->t_root.b_type = NODE;
	t->t_root.b_node = node;
}

unint ith (unint x, unint shift)
{
	return (x >> shift) & BUCKET_MASK;
}

void insert_node   (bucket_s *bucket, u32 x, u32 shift);
void insert_bucket (bucket_s *bucket, u32 x, u32 shift);

void insert_empty (bucket_s *bucket, u32 x, u32 shift)
{
	bucket->b_type = LEAF;
	bucket->b_num  = x;
}

void insert_leaf (bucket_s *bucket, u32 x, u32 shift)
{
	node_s	*node;
	u32	y = bucket->b_num;

	if (y == x) {
		//printf("Duplicate = %u\n", x);
		return;
	}
	++Nodes;
	node = ezalloc(sizeof(node_s));
	bucket->b_type = NODE;
	bucket->b_node = node;

	insert_node(bucket, y, shift);
	insert_node(bucket, x, shift);
}

void insert_node (bucket_s *bucket, u32 x, u32 shift)
{
	node_s	*node = bucket->b_node;
	unint	i = ith(x, shift);

	insert_bucket( &node->n_bucket[i], x, shift - BUCKET_SHIFT);
}

void insert_bucket (bucket_s *bucket, u32 x, u32 shift)
{
	switch (bucket->b_type) {
	case EMPTY:	insert_empty(bucket, x, shift);	return;
	case NODE:	insert_node(bucket, x, shift);	return;
	case LEAF:	insert_leaf(bucket, x, shift);	return;
	default:	printf("bad type %ld", bucket->b_type);	return;
	}
}

void insert (tree_s *tree, u32 x)
{
	bucket_s	*bucket = &tree->t_root;

	++Adds;
	insert_bucket(bucket, x, START_SHIFT);
}

void pr_bucket (bucket_s *b, unint depth);

void pr_depth (unint depth)
{
	while (depth--) {
		printf("  ");
	}
}

void pr_empty (unint depth)
{
	pr_depth(depth);
	printf("EMPTY\n");
}

void pr_leaf (u32 x, unint depth)
{
	pr_depth(depth);
	printf("%u\n", x);
}

void pr_node (node_s *node, unint depth)
{
	unint	i;

	for (i = 0; i < NUM_BUCKETS; i++) {
		pr_bucket( &node->n_bucket[i], depth+1);
	}
}

void pr_bucket (bucket_s *b, unint depth)
{
	switch (b->b_type) {
	case EMPTY:	pr_empty(depth);		return;
	case NODE:	pr_node(b->b_node, depth);	return;
	case LEAF:	pr_leaf(b->b_num, depth);	return;
	}
}

void pr_tree (tree_s *t)
{
	pr_bucket( &t->t_root, 0);
}

void usage (void)
{
	printf("Usage: %s <iterations>\n", getprogname());
	exit(2);
}

int main (int argc, char *argv[])
{
	tree_s	tree;
	u32	x;
	unint	n;
	unint	i;

	setprogname(argv[0]);

	if (argc < 2) usage();
	n = atoi(argv[1]);

	init_tree( &tree);

	for (i = 0; i < n; i++) {
		x = random() * random();
		insert( &tree, x);
	}
//	pr_tree( &tree);
	printf("Adds = %lld Nodes = %lld\n", Adds, Nodes);
	return 0;
}
