/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>
#include <token.h>

/*
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
 * ------------
 * string
 *	""
 *	" chars "
 * chars
 *	char
 *	char chars
 * char
 *	any-Unicode-character-
 *	    except-"-or-\-or-
 *	    control-character
 *	\"
 *	\\
 *	\/
 *	\b
 *	\f
 *	\n
 *	\r
 *	\t
 *	\u four-hex-digits
 * number
 *	int
 *	int frac
 *	int exp
 *	int frac exp
 * int
 *	digit
 *	digit1-9 digits 
 *	- digit
 *	- digit1-9 digits
 * frac
 *	. digits
 * exp
 *	e digits
 * digits
 *	digit
 *	digit digits
 * e
 *	e
 *	e+
 *	e-
 *	E
 *	E+
 *	E-
 */

enum {	T_STRING = 256,
	T_NUMBER,
	T_TRUE,
	T_FALSE,
	T_NULL,
	T_EOF };

typedef struct Token_s {
	int	type;
	union {
		char	*string;
		double	real;
	};
} Token_s;	

static FILE *Fp;

static void open_file (char *file)
{
	if (Fp) fatal("File already open");
	Fp = fopen(file, "r");
	if (!Fp) fatal("couldn't open %s", file);
}

static void close_file (void)
{
	fclose(Fp);
	Fp = NULL;
}

static int get (void)
{
	return getc(Fp);
}

void unget (int c)
{
	ungetc(c, Fp);
}

static Token_s get_string (void)
{
	Token_s	t;

	char	string[MAX_STRING];
	char	*s = string;

	for (;;) {
		c = get();
		switch (c) {
		case EOF:
			t.type = T_EOF;
			t.string = NULL;
			bad("Unexpected EOF, looking for '\"');
			return t;
		case '"':
			*s = '\0';
			t.type = T_STRING;
			t.string = strdup(s);
			return t;
		case '\\':
		default:
		}
	}
}

static Token_s get_token (void)
{
	Token_t	t;

	for (;;) {
		c = get();
		switch (c) {
		case EOF:
			t.type = T_EOF;
			t.string = NULL;
			return t;
		case '{':
		case '}':
		case '[':
		case ']':
		case ',':
		case ':':
			t.type = c;
			t.string = NULL;
			return t;
		case '"':
			return get_string();
		default:
			if (isdigit(c) || (c == '-')) {
				return get_number(c);
			} else {
				return get_id(c);
			}
		}
	}
}

static bool members (void)
{
	return TRUE;
}

static bool object (void)
{
	int c = get();
	if (c == '{') {
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
