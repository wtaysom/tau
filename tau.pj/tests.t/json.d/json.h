/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _JSON_H_
#define _JSON_H_ 1

enum {	T_STRING = 256,
	T_NUMBER,
	T_ARRAY,
	T_OBJECT,
	T_PAIR,
	T_TRUE,
	T_FALSE,
	T_NULL,
	T_EOF,
	T_ERR };

typedef struct Token_s {
	int	type;
	union {
		char	*string;
		double	number;
	};
} Token_s;

typedef struct Tree_s	Tree_s;
struct Tree_s {
	Token_s	token;
	Tree_s	*left;
	Tree_s	*right;
};

void open_file(char *file);
void close_file(void);
int get(void);
void unget(int c);

Token_s get_token(void);
void unget_token(Token_s t);
void pr_token(Token_s t);

void pr_json(Tree_s *tree);
void dump_json(Tree_s *tree);

#endif
