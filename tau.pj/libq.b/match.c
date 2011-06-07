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
#include <style.h>

bool isPattern (char *p)
{
	for (;;) {
		switch (*p) {
		case '\0':	return FALSE;

		case '*':	return TRUE;
		case '?':	return TRUE;

		case '\\':	++p;
				if (*p == '\0') return FALSE;
				break;

		default:	break;
		}
		++p;
	}
}

bool isMatch (char *p, char *s)
{
	for (;;) {
		switch (*p) {
		case '\0':	return *s == '\0';

		case '*':	for (;;) {
					++p;
					if (*p == '\0') return TRUE;
					if (*p != '*') break;
				}
				for (;;) {
					if (isMatch(p, s)) return TRUE;
					++s;
					if (*s == '\0') return FALSE;
				}

		case '\\':	++p;
				if (*p == '\0') return FALSE;
				if (*p != *s) return FALSE;
				break;

		case '?':	if (*s == '\0') return FALSE;
				++s;
				break;

		default:	if (*p != *s) return FALSE;
				++s;
				break;
		}
		++p;
	}
}

#if 0

struct Test_s {
	bool	isMatch;
	char	*pattern;
	char	*string;
} Tests[] = {
	{ TRUE,		"",			""		},
	{ TRUE,		"*",		""		},
	{ FALSE,	"?",		""		},
	{ TRUE,		"?",		"a"		},
	{ TRUE,		"abc",		"abc"	},
	{ TRUE,		"a*",		"abc"	},
	{ TRUE,		"***b*",	"abc"	},
	{ TRUE,		"*c",		"abc"	},
	{ TRUE,		"??c",		"abc"	},
	{ FALSE,	"??c",		"bc"	},
	{ TRUE,		"?c",		"bc"	},
	{ TRUE,		"abc",		"abc"	},
	{ TRUE,		"a***b*c",	"abc"	},
	{ FALSE,	NULL,		NULL	}
};

int main (int argc, char *argv[])
{
	struct Test_s	*test;

	for (test = Tests; test->pattern; ++test)
	{
		if (isMatch(test->pattern, test->string) != test->isMatch)
		{
			if (test->isMatch)
			{
				printf("Error: %s didn't match the pattern %s\n",
							test->string, test->pattern);
			}
			else
			{
				printf("Error: %s matched the pattern %s\n",
							test->string, test->pattern);
			}
			return 1;
		}
	}
	return 0;
}
#endif
