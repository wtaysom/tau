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

/*
 *	Cstrings finds all the string definitions in
 *	a C program feed into the program via stdin and
 *	prints the found strings to stdout.
 *
 *	Usage: Cstring <file
 */

int main (int argc, char *argv[])
{
	int			c;

	for (;;) {
		c = getchar();

		if (c == EOF) {
			exit(0);
		} else if (c == '\\') {
			c = getchar();
		} else if (c == '\"') {
			for (;;) {
				putchar(c);
				c = getchar();

				if (c == EOF) {
					fprintf(stderr,
						"reached EOF without finding \"\n");
					exit(1);
				} else if (c == '\\') {
					putchar(c);
					c = getchar();
				} else if (c == '\"') {
					putchar(c);
					putchar('\n');
					break;
				}
			}
		}
	}
}
