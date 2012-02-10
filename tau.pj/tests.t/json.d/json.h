/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef _JSON_H_
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

void open_file(char *file);
void close_file(void);
int get(void);
void unget(int c);

#endif
