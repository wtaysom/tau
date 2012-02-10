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

typedef struct Tree_s	Tree_s;
struct Tree_s {
	Token_s	token;
	Tree_s	*left;
	Tree_s	*right;
};	

static bool get_value (void)
{
	Token_s	t;
	
	t = get_token();
	switch (t.type) {
	}
	return TRUE;
}

static Tree_s *pair (void)
{
	Token_s	t;

	t = get_token();
	if (t.type != T_STRING) {
		unget_token(t);
		return NULL;
	}
	t = get_token();
	if (t.type != ':') {
		warn("Expecting ':'");
		return FALSE;
	}
	t = get_value();
	if (t.type == T_ERR) {
		warn("Expecting value");
		return FALSE;
	}
	return TRUE;
}

static Tree_s *members (void)
{
	Tree_s	*tree = NULL;

	for (;;) {
		pair();
		return TRUE;
	}
}

static Tree_s *object (void)
{
	Tree_s	*tree;

	Token_s t = get_token();
	if (t.type != '{') return NULL;
	root = new_tree(T_OBJECT);
	for (;;) {
		members();
		c = get();
		if (c == '}') return TRUE;
		return FALSE;
	}
	return FALSE;
}

void json (char *file)
{
	open_file(file);
	if (object()) {
		printf("is Object\n");
	} else {
		printf("is not object\n");
	}
	close_file();
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
