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

Tree_s *new_tree (void)
{
FN;
	Tree_s	*tree = ezalloc(sizeof(*tree));
	return tree;
}
	
static Tree_s *value(void);
static Tree_s *object(void);

static Tree_s *array (void)
{
FN;
	Tree_s	*tree = NULL;
	Token_s	t;
	Tree_s	*v;
	Tree_s	*a;

	for (;;) {
		t = get_token();
		if (t.type == ']') break;
		unget_token(t);
		v = value();
		if (tree) {
			tree->right = v;
		} else {
			tree = v;
		}
		t = get_token();
		if (t.type == ',') continue;
	}
	a = new_tree();
	a->token.type = T_ARRAY;
	a->left = tree;
	return a;
}

static Tree_s *value (void)
{
FN;
	Token_s	t;
	Tree_s	*tree;
	
	t = get_token();
	switch (t.type) {
	case T_STRING:
	case T_NUMBER:
	case T_TRUE:
	case T_FALSE:
	case T_NULL:
		tree = new_tree();
		tree->token = t;
		return tree;
	case '[':
		return array();
	case '{':
		unget_token(t);
		return object();
	default:
		return NULL;
	}
}

static Tree_s *pair (void)
{
FN;
	Tree_s	*tree;
	Token_s	s;
	Token_s	t;
	Tree_s	*v;
	Tree_s	*p;

	s = get_token();
	if (s.type != T_STRING) {
		unget_token(s);
		return NULL;
	}
	t = get_token();
	if (t.type != ':') {
		unget_token(t);
		warn("Expecting ':'");
		return NULL;
	}
	v = value();
	if (!v) {
		warn("Expecting value");
		return NULL;
	}
	tree = new_tree();
	tree->token = s;
	tree->right = v;
	p = new_tree();
	p->token.type = T_PAIR;
	p->left = tree;
	return p;
}

static Tree_s *object (void)
{
FN;
	Tree_s	*tree = NULL;
	Tree_s	*obj;
	Tree_s	*p;
	Token_s	t;

	t = get_token();
	if (t.type != '{') return NULL;
	for (;;) {
		p = pair();
		if (!p) break;
		if (tree) {
			tree->right = p;
		} else {
			tree = p;
		}
		t = get_token();
		if (t.type == ',') continue;
	}
	t = get_token();
	assert(t.type == '}');
	obj = new_tree();
	obj->token.type = T_OBJECT;
	obj->left = tree;
	return obj;
}

void json (char *file)
{
FN;
	Tree_s	*tree;

	open_file(file);
	tree = object();
	if (tree) {
		printf("is Object\n");
	} else {
		printf("is not object\n");
	}
	close_file();
	dump_json(tree);
	pr_json(tree);
}
	
int main (int argc, char *argv[])
{
	int	i;

	if (argc == 1) {
		json("/dev/tty");
	} else {
		for (i = 1; i < argc; i++) {
			json(argv[i]);
		}
	}
	return 0;
}
