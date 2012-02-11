/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

#include "json.h"

/*
 * Syntax taken from www.json.org
 *
 * object
 *	{}
 *	{ members }
 * members
 *	pair
 *	pair , members
 * pair
 *	string : value
 * array
 *	[]
 *	[ elements ]
 * elements
 *	value 
 *	value , elements
 * value
 *	string
 *	number
 *	object
 *	array
 *	true
 *	false
 *	null
 */
	
static void pr_value(Tree_s *tree, int indent);
static void pr_object(Tree_s *tree, int indent);

static void pr_indent (int indent)
{
	while (indent--) {
		printf("  ");
	}
}

static void pr_array (Tree_s *tree, int indent)
{
	assert(tree->token.type == T_ARRAY);
	pr_indent(indent); printf("[\n");
	tree = tree->left;
	while (tree) {
		pr_value(tree, indent + 1);
		tree = tree->right;
	}
	pr_indent(indent); printf("]\n");
}

static void pr_value (Tree_s *tree, int indent)
{
	switch (tree->token.type) {
	case T_STRING:
		printf("\"%s\"", tree->token.string);
		break;
	case T_NUMBER:
		printf("%g", tree->token.number);
		break;
	case T_TRUE:
		printf("true");
		break;
	case T_FALSE:
		printf("false");
		break;
	case T_NULL:
		printf("null");
		break;
	case T_ARRAY:
		pr_array(tree, indent);
		break;
	case T_OBJECT:
		pr_object(tree, indent);
		break;
	default:
		printf("Don't know how to print %d\n",
			tree->token.type);
		break;
	}
}

static void pr_pair (Tree_s *tree, int indent)
{
	assert(tree->token.type = T_PAIR);
	tree = tree->left;
	printf("\"%s\" : ", tree->token.string);
	pr_value(tree->right, indent + 1);
	printf("\n");
}

static void pr_object (Tree_s *tree, int indent)
{
	assert(tree->token.type == T_OBJECT);
	pr_indent(indent); printf("{\n");
	tree = tree->left;
	while (tree) {
		pr_pair(tree, indent + 1);
		tree = tree->right;
	}
	pr_indent(indent); printf("}\n");
}

void pr_json (Tree_s *tree)
{
	pr_object(tree, 0);
}

static void dump_tree (Tree_s *tree, int indent)
{
	if (!tree) {
		pr_indent(indent);
		printf("<nil>\n");
		return;
	}
	dump_tree(tree->left, indent + 1);
	pr_indent(indent);
	switch (tree->token.type) {
	case T_STRING:
		printf("\"%s\"\n", tree->token.string);
		break;
	case T_NUMBER:
		printf("%g\n", tree->token.number);
		break;
	case T_TRUE:
		printf("true\n");
		break;
	case T_FALSE:
		printf("false\n");
		break;
	case T_NULL:
		printf("null\n");
		break;
	case T_PAIR:
		printf("pair\n");
		break;
	case T_ARRAY:
		printf("array\n");
		break;
	case T_OBJECT:
		printf("object\n");
		break;
	default:
		printf("%d\n", tree->token.type);
		break;
	}
	dump_tree(tree->right, indent + 1);
}

void dump_json (Tree_s *tree)
{
	dump_tree(tree, 0);
}
