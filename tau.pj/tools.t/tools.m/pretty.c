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

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define TRUE	1
#define FALSE	0

#define MAX_TOKEN	513

#define isCfirst(c)	(isalpha(c) || ((c) == '_'))
#define isCchar(c)	(isalnum(c) || ((c) == '_'))

char *Reserved[] =
{
	"auto",		"break",	"case",		"char",		"const",
	"continue",	"default",	"do",		"double",	"else",
	"enum",		"extern",	"float",	"for",		"goto",
	"if",		"int",		"long",		"register",	"return",
	"short",	"signed",	"sizeof",	"static",	"struct",
	"switch",	"typedef",	"union",	"unsigned",	"void",
	"volatile",	"while",

	"include",	"if",		"ifdef",	"ifndef",	"elif",
	"else",		"defined",	"error",	"line",		"pragma",
	"define",	"endif",

	NULL
};

/*
 * This is a test comment:  static
 */

int reserved (char *token)
{
	unsigned	i;

	for (i = 0; Reserved[i] != 0; ++i) {
		if (strcmp(Reserved[i], token) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}


void boldChar (int c)
{
	unsigned	i;

	putchar(c);
	for (i = 0; i < 3; ++i) {
		putchar('\b');
		putchar(c);
	}
}

void bold (char *string)
{
	while (*string) {
		boldChar(*string++);
	}
}

void plain (char *string)
{
	while (*string) {
		putchar(*string++);
	}
}

char	Token[MAX_TOKEN];

int main (int argc, char *argv[])
{
	int		c;
	char	*token;

	for (;;) {
		c = getchar();

		if (isCfirst(c)) {
			token = Token;
			*token++ = c;
			for (;;) {
				c = getchar();

				if (isCchar(c)) {
					*token++ = c;
				} else {
					*token = '\0';
					if (reserved(Token)) {
						bold(Token);
					} else {
						plain(Token);
					}
					if (c == EOF) {
						return 0;
					}
					break;	
				}
			}
		}
		if (c == '/')
		{
			putchar(c);
			c = getchar();
			if (c == '*') {
				putchar(c);
				for (;;) {
					c = getchar();
					while (c == '*') {
						putchar(c);
						c = getchar();
						if ((c == '/') || (c == EOF)) {
							goto next;
						}
					}
					putchar(c);
				}
			}
		}
		if ((c == '{') || (c == '}')) {
			boldChar(c);
			continue;
		}
		if (c == '\"') {
			putchar(c);
			for (;;) {
				c = getchar();
				if (c == '\\') {
					putchar(c);
					c = getchar();
					putchar(c);
					continue;
				}
				if ((c == '\"') || (c == EOF)) {
					break;
				}
				putchar(c);
			}
		}
		if (c == '\'') {
			putchar(c);
			for (;;) {
				c = getchar();
				if (c == '\\') {
					putchar(c);
					c = getchar();
					putchar(c);
					continue;
				}
				if ((c == '\'') || (c == EOF)) {
					break;
				}
				putchar(c);
			}
		}
next:
		if (c == EOF) {
			return 0;
		}
		putchar(c);
	}
}
