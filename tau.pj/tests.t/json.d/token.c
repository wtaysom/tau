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
 * Lexicon taken from www.json.org
 *
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

enum {	MAX_STRING = 4096 };

typedef struct Id_s {
	int	type;
	char	*name;
} Id_s;

static Id_s	Id[] = {
	{ T_NULL, "null" },
	{ T_TRUE, "true" },
	{ T_FALSE, "false" },
	{ 0, NULL }};


static Token_s token (unint type, char *string)
{
	Token_s	t;
	
	t.type = type;
	t.string = string;
	return t;
}

static Token_s a_number (char *string)
{
	Token_s	t;
	
	t.type = T_NUMBER;
	t.number = atof(string);
	return t;
}

static Token_s t_err (char *err)
{
	return token(T_ERR, err);
}

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

static bool get_unicode (char **sp)
{
	char	*s = *sp;
	unint	x = 0;
	int	c;
	int	i;

	for (i = 0; i < 4; i++) {
		c = get();
		if (isxdigit(c)) {
			x <<= 4;
			if (isdigit(c)) {
				x |= c - '0';
			} else if (isupper(c)) {
				x |= c - 'A';
			} else {
				x |= c - 'a';
			}
		} else {
			warn("Expected hex digit");
			unget(c);
			return FALSE;
		}
	}
	if (x < 128) {
		*s++ = x;
	} else {
		warn("Unicode %lu", x);
		return FALSE;
	}
	*sp = s;
	return TRUE;
}

static Token_s get_id (void)
{
	char	string[MAX_STRING+6];
	char	*s = string;
	char	*end = &string[MAX_STRING];
	int	c;
	int	i;

	for (;;) {
		if (s >= end) {
			*s = '\0';
			warn("string too long %s", string);
			return t_err("string too long");
		}
		c = get();
		switch (c) {
		case EOF:
			warn("Unexpected EOF, looking for '\"'");
			return t_err("Unexpected EOF");
		default:
			if (isspace(c)) {
				*s = '\0';
				for (i = 0; Id[i].name; i++) {
					if (strcmp(Id[i].name, string) == 0) {
						return token(Id[i].type, NULL);
					}
				}
				warn("expecting null, true or fals: %s", string);
				return t_err("unknow id");
			} else {
				*s++ = c;
			}
		}
	}
}
		
static Token_s get_number (void)
{
	char	string[MAX_STRING+6];
	char	*s = string;
	char	*end = &string[MAX_STRING];
	int	c;

	for (;;) {
		if (s >= end) {
			*s = '\0';
			warn("string too long %s", string);
			return t_err("string too long");
		}
		c = get();
		if (isdigit(c)) {
			*s++ = c;
		} else {
			*s = '\0';
			unget(c);
			return a_number(string);
		}
	}
}			
	

static Token_s get_string (void)
{
	Token_s	t;
	char	string[MAX_STRING+6];
	char	*s = string;
	char	*end = &string[MAX_STRING];
	int	c;

	for (;;) {
		if (s >= end) {
			*s = '\0';
			warn("string too long %s", string);
			return t_err("string too long");
		}
		c = get();
		switch (c) {
		case EOF:
			warn("Unexpected EOF, looking for '\"'");
			return t_err("Unexpected EOF");
		case '"':
			*s = '\0';
			t.type = T_STRING;
			t.string = strdup(s);
			return t;
		case '\\':
			c = get();
			switch (c) {
			case EOF:
				warn("Unexpected EOF");
				return t_err("Unexpected EOF");
			case '"':  *s++ = '"';  break;
			case '\\': *s++ = '\\'; break;
			case '/':  *s++ = '/';  break;
			case 'b':  *s++ = '\b'; break;
			case 'f':  *s++ = '\f'; break;
			case 'n':  *s++ = '\n'; break;
			case 'r':  *s++ = '\r'; break;
			case 't':  *s++ = '\t'; break;
			case 'u':
				if (get_unicode(&s)) {
					break;
				} else {
					return t_err("Expected hex digit");
				}
			default:
				*s++ = c;
				break;
			}
		default:
			*s++ = c;
			break;
		}
	}
}

static Token_s get_token (void)
{
	int	c;

	for (;;) {
		c = get();
		switch (c) {
		case EOF:
			return token(T_EOF, NULL);
		case '{':
		case '}':
		case '[':
		case ']':
		case ',':
		case ':':
			return token(c, NULL);
		case '"':
			return get_string();
		default:
			unget(c);
			if (isdigit(c) || (c == '-')) {
				return get_number();
			} else {
				return get_id();
			}
		}
	}
}
